
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
// Copyright (c) 2014-2018 Francois Beaune, The appleseedhq Organization
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

#pragma once

// appleseed.renderer headers.
#include "renderer/kernel/intersection/probevisitorbase.h"
#include "renderer/kernel/intersection/treerepository.h"
#include "renderer/kernel/intersection/triangletree.h"
#include "renderer/kernel/shading/shadingray.h"
#include "renderer/modeling/scene/assembly.h"
#include "renderer/modeling/scene/containers.h"
#include "renderer/utility/transformsequence.h"

// appleseed.foundation headers.
#include "foundation/core/concepts/noncopyable.h"
#include "foundation/math/aabb.h"
#include "foundation/math/bvh.h"
#include "foundation/utility/alignedvector.h"
#include "foundation/utility/uid.h"
#include "foundation/utility/version.h"

// Standard headers.
#include <cstddef>
#include <map>
#include <vector>

// Forward declarations.
namespace foundation    { class Statistics; }
namespace renderer      { class AssemblyInstance; }
namespace renderer      { class Scene; }
namespace renderer      { class ShadingPoint; }

namespace renderer
{

//
// Assembly tree.
//

class InstanceTree
  : public foundation::bvh::Tree<
               foundation::AlignedVector<
                   foundation::bvh::Node<foundation::AABB3d>
               >
           >
{
  public:
    // Constructor, builds the tree for a given scene.
    explicit InstanceTree(const Scene& scene);

    // Destructor.
    ~InstanceTree();

    // Update the instance tree and all the child trees.
    void update();

    // Return the size (in bytes) of this object in memory.
    size_t get_memory_size() const;

  private:
    friend class InstanceLeafVisitor;
    friend class InstanceLeafProbeVisitor;
    friend class Intersector;

    struct Item
    {
        const renderer::Assembly*               m_assembly;
        foundation::UniqueID                    m_assembly_uid;
        const renderer::AssemblyInstance*       m_assembly_instance;
        renderer::TransformSequence             m_transform_sequence;

        Item() {}

        Item(
            const renderer::Assembly*           assembly,
            const renderer::AssemblyInstance*   assembly_instance,
            const renderer::TransformSequence&  transform_sequence)
          : m_assembly(assembly)
          , m_assembly_uid(assembly->get_uid())
          , m_assembly_instance(assembly_instance)
          , m_transform_sequence(transform_sequence)
        {
        }
    };

    typedef std::vector<Item> ItemVector;
    typedef std::vector<foundation::AABB3d> AABBVector;
    typedef std::vector<const Assembly*> AssemblyVector;
    typedef std::map<foundation::UniqueID, foundation::VersionID> AssemblyVersionMap;

    const Scene&                    m_scene;
    ItemVector                      m_items;
    AssemblyVersionMap              m_assembly_versions;

    TreeRepository<TriangleTree>    m_triangle_tree_repository;
    TriangleTreeContainer           m_triangle_trees;

    void collect_assembly_instances(
        const AssemblyInstanceContainer&        assembly_instances,
        const TransformSequence&                parent_transform_seq,
        AABBVector&                             assembly_instance_bboxes);

    void rebuild_instance_tree();
    void store_items_in_leaves(foundation::Statistics& statistics);

    void update_tree_hierarchy();
    void collect_unique_assemblies(AssemblyVector& assemblies) const;
    void delete_unused_child_trees(const AssemblyVector& assemblies);

    void create_child_trees(const Assembly& assembly);
    void create_triangle_tree(const Assembly& assembly);

    void delete_child_trees(const foundation::UniqueID assembly_id);
    void delete_triangle_tree(const foundation::UniqueID assembly_id);

    void update_triangle_trees();
};


//
// Instance leaf visitor, used during tree intersection.
//

class InstanceLeafVisitor
  : public foundation::NonCopyable
{
  public:
    // Constructor.
    InstanceLeafVisitor(
        ShadingPoint&                               shading_point,
        const InstanceTree&                         tree,
        TriangleTreeAccessCache&                    triangle_tree_cache,
        const ShadingPoint*                         parent_shading_point
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
        , foundation::bvh::TraversalStatistics&     triangle_tree_stats
#endif
        );

    // Visit a leaf.
    bool visit(
        const InstanceTree::NodeType&               node,
        const ShadingRay&                           ray,
        const ShadingRay::RayInfoType&              ray_info,
        double&                                     distance
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
        , foundation::bvh::TraversalStatistics&     stats
#endif
        );

  private:
    ShadingPoint&                                   m_shading_point;
    const InstanceTree&                             m_tree;
    TriangleTreeAccessCache&                        m_triangle_tree_cache;
    const ShadingPoint*                             m_parent_shading_point;
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
    foundation::bvh::TraversalStatistics&           m_triangle_tree_stats;
#endif
};


//
// Instance leaf visitor for probe rays, only return boolean answers
// (whether an intersection was found or not).
//

class InstanceLeafProbeVisitor
  : public ProbeVisitorBase
{
  public:
    // Constructor.
    InstanceLeafProbeVisitor(
        const InstanceTree&                         tree,
        TriangleTreeAccessCache&                    triangle_tree_cache,
        const ShadingPoint*                         parent_shading_point
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
        , foundation::bvh::TraversalStatistics&     triangle_tree_stats
#endif
        );

    // Visit a leaf.
    bool visit(
        const InstanceTree::NodeType&               node,
        const ShadingRay&                           ray,
        const ShadingRay::RayInfoType&              ray_info,
        double&                                     distance
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
        , foundation::bvh::TraversalStatistics&     stats
#endif
        );

  private:
    const InstanceTree&                             m_tree;
    TriangleTreeAccessCache&                        m_triangle_tree_cache;
    const ShadingPoint*                             m_parent_shading_point;
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
    foundation::bvh::TraversalStatistics&           m_triangle_tree_stats;
#endif
};


//
// Assembly tree intersectors.
//

typedef foundation::bvh::Intersector<
    InstanceTree,
    InstanceLeafVisitor,
    ShadingRay
> InstanceTreeIntersector;

typedef foundation::bvh::Intersector<
    InstanceTree,
    InstanceLeafProbeVisitor,
    ShadingRay
> InstanceTreeProbeIntersector;


//
// InstanceLeafVisitor class implementation.
//

inline InstanceLeafVisitor::InstanceLeafVisitor(
    ShadingPoint&                                   shading_point,
    const InstanceTree&                             tree,
    TriangleTreeAccessCache&                        triangle_tree_cache,
    const ShadingPoint*                             parent_shading_point
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
    , foundation::bvh::TraversalStatistics&         triangle_tree_stats
#endif
    )
  : m_shading_point(shading_point)
  , m_tree(tree)
  , m_triangle_tree_cache(triangle_tree_cache)
  , m_parent_shading_point(parent_shading_point)
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
  , m_triangle_tree_stats(triangle_tree_stats)
#endif
{
}


//
// InstanceLeafProbeVisitor class implementation.
//

inline InstanceLeafProbeVisitor::InstanceLeafProbeVisitor(
    const InstanceTree&                             tree,
    TriangleTreeAccessCache&                        triangle_tree_cache,
    const ShadingPoint*                             parent_shading_point
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
    , foundation::bvh::TraversalStatistics&         triangle_tree_stats
#endif
    )
  : m_tree(tree)
  , m_triangle_tree_cache(triangle_tree_cache)
  , m_parent_shading_point(parent_shading_point)
#ifdef FOUNDATION_BVH_ENABLE_TRAVERSAL_STATS
  , m_triangle_tree_stats(triangle_tree_stats)
#endif
{
}

}   // namespace renderer
