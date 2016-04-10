
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

#ifndef APPLESEED_RENDERER_KERNEL_INTERSECTION_PATCHTREE_H
#define APPLESEED_RENDERER_KERNEL_INTERSECTION_PATCHTREE_H

// appleseed.renderer headers.
#include "renderer/global/globaltypes.h"
#include "renderer/kernel/intersection/patchkey.h"
#include "renderer/kernel/intersection/intersectionsettings.h"
#include "renderer/kernel/intersection/probevisitorbase.h"
#include "renderer/kernel/shading/shadingpoint.h"
#include "renderer/kernel/shading/shadingray.h"

// appleseed.foundation headers.
#include "foundation/core/concepts/noncopyable.h"
#include "foundation/math/bvh.h"
#include "foundation/platform/types.h"
#include "foundation/utility/alignedvector.h"
#include "foundation/utility/lazy.h"
#include "foundation/utility/poolallocator.h"
#include "foundation/utility/uid.h"

// Standard headers.
#include <cstddef>
#include <map>
#include <memory>
#include <vector>

// Forward declarations.
namespace foundation    { class Statistics; }
namespace renderer      { class Assembly; }
namespace renderer      { class ParamArray; }
namespace renderer      { class Scene; }

namespace renderer
{

//
// Patch tree.
//

class PatchTree
  : public foundation::bvh::Tree<
               foundation::AlignedVector<
                   foundation::bvh::Node<GAABB3>
               >
           >
{
  public:
    // Construction arguments.
    struct Arguments
    {
        const Scene&                            m_scene;
        const foundation::UniqueID              m_patch_tree_uid;
        const GAABB3                            m_bbox;
        const Assembly&                         m_assembly;

        // Constructor.
        Arguments(
            const Scene&                        scene,
            const foundation::UniqueID          patch_tree_uid,
            const GAABB3&                       bbox,
            const Assembly&                     assembly);
    };

    // Constructor, builds the tree for a given assembly.
    explicit PatchTree(const Arguments& arguments);

  private:
    friend class PatchLeafVisitor;
    friend class PatchLeafProbeVisitor;

    struct LeafUserData
    {
        /*
        foundation::uint32  m_patch1_offset;
        foundation::uint32  m_patch1_count;
        foundation::uint32  m_patch3_offset;
        foundation::uint32  m_patch3_count;
        */
    };

    const Arguments         m_arguments;
    //std::vector<PatchType1> m_patchs1;
    //std::vector<PatchType3> m_patchs3;
    std::vector<PatchKey>   m_patch_keys;

    void collect_patches(std::vector<GAABB3>& patch_bboxes);

    void build_bvh(
        const ParamArray&                       params,
        const double                            time,
        foundation::Statistics&                 statistics);

    // Reorder patch keys to match a given ordering.
    void reorder_patch_keys(const std::vector<size_t>& ordering);

    // Reorder patchs to match a given ordering.
    void reorder_patches(const std::vector<size_t>& ordering);

    // Reorder patch keys in leaf nodes so that all degree-1 patch keys come before degree-3 ones.
    void reorder_patch_keys_in_leaf_nodes();
};


//
// Patch tree factory.
//

class PatchTreeFactory
  : public foundation::ILazyFactory<PatchTree>
{
  public:
    // Constructor.
    explicit PatchTreeFactory(
        const PatchTree::Arguments&  arguments);

    // Create the patch tree.
    virtual std::auto_ptr<PatchTree> create();

  private:
    const PatchTree::Arguments       m_arguments;
};


//
// Some additional types.
//

// Patch tree container and iterator types.
typedef std::map<
    foundation::UniqueID,
    foundation::Lazy<PatchTree>*
> PatchTreeContainer;
typedef PatchTreeContainer::iterator PatchTreeIterator;
typedef PatchTreeContainer::const_iterator PatchTreeConstIterator;

// Patch tree access cache type.
typedef foundation::AccessCacheMap<
    PatchTreeContainer,
    PatchTreeAccessCacheLines,
    PatchTreeAccessCacheWays,
    foundation::PoolAllocator<void, PatchTreeAccessCacheLines * PatchTreeAccessCacheWays>
> PatchTreeAccessCache;


//
// Patch leaf visitor, used during tree intersection.
//

class PatchLeafVisitor
  : public foundation::NonCopyable
{
  public:
    // Constructor.
    PatchLeafVisitor(
        const PatchTree&                        tree,
        const PatchMatrixType&                  xfm_matrix,
        ShadingPoint&                           shading_point);

    // Visit a leaf.
    bool visit(
        const PatchTree::NodeType&              node,
        const GRay3&                            ray,
        const GRayInfo3&                        ray_info,
        GScalar&                                distance
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
        , foundation::bvh::TraversalStatistics& stats
#endif
        );

  private:
    const PatchTree&                            m_tree;
    const PatchMatrixType&                      m_xfm_matrix;
    ShadingPoint&                               m_shading_point;
};


//
// Patch leaf visitor for probe rays, only return boolean answers
// (whether an intersection was found or not).
//

class PatchLeafProbeVisitor
  : public ProbeVisitorBase
{
  public:
    // Constructor.
    PatchLeafProbeVisitor(
        const PatchTree&                        tree,
        const PatchMatrixType&                  xfm_matrix);

    // Visit a leaf.
    bool visit(
        const PatchTree::NodeType&              node,
        const GRay3&                            ray,
        const GRayInfo3&                        ray_info,
        GScalar&                                distance
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
        , foundation::bvh::TraversalStatistics& stats
#endif
        );

  private:
    const PatchTree&                            m_tree;
    const PatchMatrixType&                      m_xfm_matrix;
};


//
// Patch tree intersectors.
//

typedef foundation::bvh::Intersector<
    PatchTree,
    PatchLeafVisitor,
    GRay3,
    PatchTreeStackSize
> PatchTreeIntersector;

typedef foundation::bvh::Intersector<
    PatchTree,
    PatchLeafProbeVisitor,
    GRay3,
    PatchTreeStackSize
> PatchTreeProbeIntersector;


//
// PatchLeafVisitor class implementation.
//

inline PatchLeafVisitor::PatchLeafVisitor(
    const PatchTree&                            tree,
    const PatchMatrixType&                      xfm_matrix,
    ShadingPoint&                               shading_point)
  : m_tree(tree)
  , m_xfm_matrix(xfm_matrix)
  , m_shading_point(shading_point)
{
}

inline bool PatchLeafVisitor::visit(
    const PatchTree::NodeType&                  node,
    const GRay3&                                ray,
    const GRayInfo3&                            ray_info,
    GScalar&                                    distance
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
    , foundation::bvh::TraversalStatistics&     stats
#endif
    )
{
    /*
    const PatchTree::LeafUserData& user_data = node.get_user_data<PatchTree::LeafUserData>();

    size_t patch_index = node.get_item_index();
    size_t hit_patch_index = ~0;
    GScalar u, v, t = ray.m_tmax;

    for (foundation::uint32 i = 0; i < user_data.m_patch1_count; ++i, ++patch_index)
    {
        const PatchType1& patch = m_tree.m_patchs1[user_data.m_patch1_offset + i];
        if (Patch1IntersectorType::intersect(patch, ray, m_xfm_matrix, u, v, t))
        {
            m_shading_point.m_primitive_type = ShadingPoint::PrimitivePatch1;
            m_shading_point.m_ray.m_tmax = static_cast<double>(t);
            m_shading_point.m_bary[0] = static_cast<double>(u);
            m_shading_point.m_bary[1] = static_cast<double>(v);
            hit_patch_index = patch_index;
        }
    }

    FOUNDATION_BVH_TRAVERSAL_STATS(stats.m_intersected_items.insert(patch1_patch_count));

    for (foundation::uint32 i = 0; i < user_data.m_patch3_count; ++i, ++patch_index)
    {
        const PatchType3& patch = m_tree.m_patchs3[user_data.m_patch3_offset + i];
        if (Patch3IntersectorType::intersect(patch, ray, m_xfm_matrix, u, v, t))
        {
            m_shading_point.m_primitive_type = ShadingPoint::PrimitivePatch3;
            m_shading_point.m_ray.m_tmax = static_cast<double>(t);
            m_shading_point.m_bary[0] = static_cast<double>(u);
            m_shading_point.m_bary[1] = static_cast<double>(v);
            hit_patch_index = patch_index;
        }
    }

    FOUNDATION_BVH_TRAVERSAL_STATS(stats.m_intersected_items.insert(patch3_patch_count));

    if (hit_patch_index != size_t(~0))
    {
        const PatchKey& patch_key = m_tree.m_patch_keys[hit_patch_index];
        m_shading_point.m_object_instance_index = patch_key.get_object_instance_index();
        m_shading_point.m_primitive_index = patch_key.get_patch_index_object();
    }
    */

    // Continue traversal.
    distance = static_cast<GScalar>(m_shading_point.m_ray.m_tmax);
    return true;
}


//
// PatchLeafProbeVisitor class implementation.
//

inline PatchLeafProbeVisitor::PatchLeafProbeVisitor(
    const PatchTree&                            tree,
    const PatchMatrixType&                      xfm_matrix)
  : m_tree(tree)
  , m_xfm_matrix(xfm_matrix)
{
}

inline bool PatchLeafProbeVisitor::visit(
    const PatchTree::NodeType&                  node,
    const GRay3&                                ray,
    const GRayInfo3&                            ray_info,
    GScalar&                                    distance
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
    , foundation::bvh::TraversalStatistics&     stats
#endif
    )
{
    /*
    const PatchTree::LeafUserData& user_data = node.get_user_data<PatchTree::LeafUserData>();

    for (foundation::uint32 i = 0; i < user_data.m_patch1_count; ++i)
    {
        const PatchType1& patch = m_tree.m_patchs1[user_data.m_patch1_offset + i];
        if (Patch1IntersectorType::intersect(patch, ray, m_xfm_matrix))
        {
            FOUNDATION_BVH_TRAVERSAL_STATS(stats.m_intersected_items.insert(i + 1));
            m_hit = true;
            return false;
        }
    }

    FOUNDATION_BVH_TRAVERSAL_STATS(stats.m_intersected_items.insert(patch1_patch_count));

    for (foundation::uint32 i = 0; i < user_data.m_patch3_count; ++i)
    {
        const PatchType3& patch = m_tree.m_patchs3[user_data.m_patch3_offset + i];
        if (Patch3IntersectorType::intersect(patch, ray, m_xfm_matrix))
        {
            FOUNDATION_BVH_TRAVERSAL_STATS(stats.m_intersected_items.insert(i + 1));
            m_hit = true;
            return false;
        }
    }

    FOUNDATION_BVH_TRAVERSAL_STATS(stats.m_intersected_items.insert(patch3_patch_count));
    */
    // Continue traversal.
    distance = ray.m_tmax;
    return true;
}

}       // namespace renderer

#endif  // !APPLESEED_RENDERER_KERNEL_INTERSECTION_PATCHTREE_H
