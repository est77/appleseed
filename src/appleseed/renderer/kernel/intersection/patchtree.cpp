
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016 Esteban Tovagliari, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "patchtree.h"

// appleseed.renderer headers.
#include "renderer/global/globallogger.h"
#include "renderer/modeling/object/subdivisionsurfaceobject.h"
#include "renderer/modeling/object/object.h"
#include "renderer/modeling/scene/assembly.h"
#include "renderer/modeling/scene/containers.h"
#include "renderer/modeling/scene/objectinstance.h"
#include "renderer/utility/messagecontext.h"
#include "renderer/utility/paramarray.h"

// appleseed.foundation headers.
#include "foundation/core/exceptions/exceptionnotimplemented.h"
//#include "foundation/math/bezierpatch.h"
#include "foundation/math/permutation.h"
#include "foundation/math/transform.h"
#include "foundation/platform/defaulttimers.h"
#include "foundation/platform/system.h"
#include "foundation/utility/alignedallocator.h"
#include "foundation/utility/makevector.h"
#include "foundation/utility/memory.h"
#include "foundation/utility/statistics.h"
#include "foundation/utility/stopwatch.h"
#include "foundation/utility/string.h"

// Standard headers.
#include <cassert>
#include <cstring>
#include <string>

using namespace foundation;
using namespace std;

namespace renderer
{

//
// PatchTree class implementation.
//

PatchTree::Arguments::Arguments(
    const Scene&            scene,
    const UniqueID          patch_tree_uid,
    const GAABB3&           bbox,
    const Assembly&         assembly)
  : m_scene(scene)
  , m_patch_tree_uid(patch_tree_uid)
  , m_bbox(bbox)
  , m_assembly(assembly)
{
}

PatchTree::PatchTree(const Arguments& arguments)
  : TreeType(AlignedAllocator<void>(System::get_l1_data_cache_line_size()))
  , m_arguments(arguments)
{
    // Retrieve construction parameters.
    const MessageContext message_context(
        string("while building patch tree for assembly \"") + m_arguments.m_assembly.get_name() + "\"");
    const ParamArray& params = m_arguments.m_assembly.get_parameters().child("acceleration_structure");
    const string algorithm = params.get_optional<string>("algorithm", "bvh", make_vector("bvh", "sbvh"), message_context);
    const double time = params.get_optional<double>("time", 0.5);

    // Start stopwatch.
    Stopwatch<DefaultWallclockTimer> stopwatch;
    stopwatch.start();

    // Build the tree.
    Statistics statistics;
    if (algorithm == "bvh")
        build_bvh(params, time, statistics);
    else throw ExceptionNotImplemented();

    // Print patch tree statistics.
    statistics.insert_size("nodes alignment", alignment(&m_nodes[0]));
    statistics.insert_time("total time", stopwatch.measure().get_seconds());
    RENDERER_LOG_DEBUG("%s",
        StatisticsVector::make(
            "patch tree #" + to_string(m_arguments.m_patch_tree_uid) + " statistics",
            statistics).to_string().c_str());
}

void PatchTree::collect_patches(vector<GAABB3>& patch_bboxes)
{
    const ObjectInstanceContainer& object_instances = m_arguments.m_assembly.object_instances();

    for (size_t i = 0; i < object_instances.size(); ++i)
    {
        // Retrieve the object instance.
        const ObjectInstance* object_instance = object_instances.get_by_index(i);
        assert(object_instance);

        // Retrieve the object.
        const Object& object = object_instance->get_object();

        // Process only patch objects.
        if (strcmp(object.get_model(), SubdivisionSurfaceObjectFactory::get_model()))
            continue;

        //const SubdivisionSurfaceObject& subdiv_object = static_cast<const SubdivisionSurfaceObject&>(object);

        // Retrieve the object instance transform.
        //const Transformd::MatrixType& transform =
        //    object_instance->get_transform().get_local_to_parent();

        /*
        // Store degree-1 patchs, patch keys and patch bounding boxes.
        const size_t patch1_count = patch_object.get_patch1_count();
        for (size_t j = 0; j < patch1_count; ++j)
        {
            const PatchType1 patch(patch_object.get_patch1(j), transform);
            const PatchKey patch_key(
                i,                  // object instance index
                j,                  // patch index in object
                m_patchs1.size(),   // patch index in tree
                0,                  // for now we assume all the patchs have the same material
                1);                 // patch degree

            GAABB3 patch_bbox = patch.compute_bbox();
            patch_bbox.grow(GVector3(GScalar(0.5) * patch.compute_max_width()));

            m_patchs1.push_back(patch);
            m_patch_keys.push_back(patch_key);
            patch_bboxes.push_back(patch_bbox);
        }

        // Store degree-3 patchs, patch keys and patch bounding boxes.
        const size_t patch3_count = patch_object.get_patch3_count();
        for (size_t j = 0; j < patch3_count; ++j)
        {
            const PatchType3 patch(patch_object.get_patch3(j), transform);
            const PatchKey patch_key(
                i,                  // object instance index
                j,                  // patch index in object
                m_patchs3.size(),   // patch index in tree
                0,                  // for now we assume all the patchs have the same material
                3);                 // patch degree

            GAABB3 patch_bbox = patch.compute_bbox();
            patch_bbox.grow(GVector3(GScalar(0.5) * patch.compute_max_width()));

            m_patchs3.push_back(patch);
            m_patch_keys.push_back(patch_key);
            patch_bboxes.push_back(patch_bbox);
        }
        */
    }
}

void PatchTree::build_bvh(
    const ParamArray&       params,
    const double            time,
    Statistics&             statistics)
{
    // Collect patchs for this tree.
    RENDERER_LOG_INFO(
        "collecting geometry for patch tree #" FMT_UNIQUE_ID " from assembly \"%s\"...",
        m_arguments.m_patch_tree_uid,
        m_arguments.m_assembly.get_name());
    vector<GAABB3> patch_bboxes;
    collect_patches(patch_bboxes);

    // Print statistics about the input geometry.
    RENDERER_LOG_INFO(
        "building patch tree #" FMT_UNIQUE_ID " (bvh, %s %s)...",
        m_arguments.m_patch_tree_uid,
        pretty_uint(m_patch_keys.size()).c_str(),
        plural(m_patch_keys.size(), "patch").c_str());

    // Create the partitioner.
    typedef bvh::SAHPartitioner<vector<GAABB3> > Partitioner;
    Partitioner partitioner(
        patch_bboxes,
        PatchTreeDefaultMaxLeafSize,
        PatchTreeDefaultInteriorNodeTraversalCost,
        PatchTreeDefaultPatchIntersectionCost);

    // Build the tree.
    typedef bvh::Builder<PatchTree, Partitioner> Builder;
    Builder builder;
    builder.build<DefaultWallclockTimer>(
        *this,
        partitioner,
        0, //m_patchs1.size() + m_patchs3.size(),
        PatchTreeDefaultMaxLeafSize);
    statistics.merge(
        bvh::TreeStatistics<PatchTree>(*this, m_arguments.m_bbox));

    /*
    // Reorder the patch keys based on the nodes ordering.
    if (!m_patchs1.empty() || !m_patchs3.empty())
    {
        const vector<size_t>& ordering = partitioner.get_item_ordering();
        reorder_patch_keys(ordering);
        reorder_patches(ordering);
        reorder_patch_keys_in_leaf_nodes();
    }
    */
}

void PatchTree::reorder_patch_keys(const vector<size_t>& ordering)
{
    vector<PatchKey> temp_keys(m_patch_keys.size());
    small_item_reorder(&m_patch_keys[0], &temp_keys[0], &ordering[0], ordering.size());
}

void PatchTree::reorder_patches(const vector<size_t>& ordering)
{
    /*
    vector<PatchType1> new_patchs1(m_patchs1.size());
    vector<PatchType3> new_patchs3(m_patchs3.size());

    size_t patch1_index = 0;
    size_t patch3_index = 0;

    for (size_t i = 0; i < ordering.size(); ++i)
    {
        const PatchKey& key = m_patch_keys[i];

        if (key.get_patch_degree() == 1)
        {
            new_patchs1[patch1_index] = m_patchs1[key.get_patch_index_tree()];
            m_patch_keys[i].set_patch_index_tree(patch1_index);
            ++patch1_index;
        }
        else
        {
            assert(key.get_patch_degree() == 3);
            new_patchs3[patch3_index] = m_patchs3[key.get_patch_index_tree()];
            m_patch_keys[i].set_patch_index_tree(patch3_index);
            ++patch3_index;
        }
    }

    assert(patch1_index == m_patchs1.size());
    assert(patch3_index == m_patchs3.size());

    m_patchs1.swap(new_patchs1);
    m_patchs3.swap(new_patchs3);
    */
}

void PatchTree::reorder_patch_keys_in_leaf_nodes()
{
    /*
    for (size_t i = 0; i < m_nodes.size(); ++i)
    {
        if (!m_nodes[i].is_leaf())
            continue;

        const size_t item_index = m_nodes[i].get_item_index();
        const size_t item_count = m_nodes[i].get_item_count();

        // Collect the patch keys for this leaf node.
        vector<PatchKey> patch1_keys;
        vector<PatchKey> patch3_keys;
        for (size_t j = 0; j < item_count; ++j)
        {
            const PatchKey& key = m_patch_keys[item_index + j];
            if (key.get_patch_degree() == 1)
                patch1_keys.push_back(key);
            else patch3_keys.push_back(key);
        }

        // Store count and start offset in the leaf node's user data.
        LeafUserData& user_data = m_nodes[i].get_user_data<LeafUserData>();
        user_data.m_patch1_offset = patch1_keys.empty() ? 0 : static_cast<uint32>(patch1_keys[0].get_patch_index_tree());
        user_data.m_patch1_count = static_cast<uint32>(patch1_keys.size());
        user_data.m_patch3_offset = patch3_keys.empty() ? 0 : static_cast<uint32>(patch3_keys[0].get_patch_index_tree());
        user_data.m_patch3_count = static_cast<uint32>(patch3_keys.size());

        // Reorder the patch keys in the original list.
        size_t output_index = item_index;
        for (size_t j = 0; j < patch1_keys.size(); ++j)
            m_patch_keys[output_index++] = patch1_keys[j];
        for (size_t j = 0; j < patch3_keys.size(); ++j)
            m_patch_keys[output_index++] = patch3_keys[j];
    }
    */
}


//
// PatchTreeFactory class implementation.
//

PatchTreeFactory::PatchTreeFactory(const PatchTree::Arguments& arguments)
  : m_arguments(arguments)
{
}

auto_ptr<PatchTree> PatchTreeFactory::create()
{
    return auto_ptr<PatchTree>(new PatchTree(m_arguments));
}

}   // namespace renderer
