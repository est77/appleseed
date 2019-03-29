
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017-2018 Petra Gospodnetic, The appleseedhq Organization
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
#include "renderer/kernel/intersection/intersectionsettings.h"
#include "renderer/utility/transformsequence.h"

// appleseed.foundation headers.
#include "foundation/math/matrix.h"
#include "foundation/math/vector.h"
#include "foundation/utility/stampedptr.h"

// Standard headers.
#include <cstddef>

// Forward declarations.
namespace renderer  { class AssemblyInstance; }
namespace renderer  { class Intersector; }
namespace renderer  { class Light; }
namespace renderer  { class LightSample; }
namespace renderer  { class Material; }
namespace renderer  { class ShadingPoint; }

namespace renderer
{

enum LightType
{
    NonPhysicalLightType = 0,
    EmittingShapeType = 1
};

//
// A non-physical light.
//

class NonPhysicalLightInfo
{
  public:
    TransformSequence           m_transform_sequence;           // assembly instance (parent of the light) space to world space
    const Light*                m_light;
};


//
// A light-emitting shape.
//

class EmittingShape
{
  public:
    enum ShapeType
    {
        DiskShape = 0,
        TriangleShape,
        SphereShape,
        RectShape
    };

    static EmittingShape create_disk_shape(
        const AssemblyInstance*     assembly_instance,
        const size_t                object_instance_index,
        const Material*             material,
        const foundation::Vector3d& center,
        const double                radius,
        const foundation::Matrix4d& object_to_world);

    static EmittingShape create_triangle_shape(
        const AssemblyInstance*     assembly_instance,
        const size_t                object_instance_index,
        const size_t                primitive_index,
        const Material*             material,
        const foundation::Vector3d& v0,
        const foundation::Vector3d& v1,
        const foundation::Vector3d& v2,
        const foundation::Vector3d& n0,
        const foundation::Vector3d& n1,
        const foundation::Vector3d& n2,
        const foundation::Vector3d& geometric_normal);

    static EmittingShape create_sphere_shape(
        const AssemblyInstance*     assembly_instance,
        const size_t                object_instance_index,
        const Material*             material,
        const foundation::Vector3d& center,
        const double                radius);

    static EmittingShape create_rect_shape(
        const AssemblyInstance*     assembly_instance,
        const size_t                object_instance_index,
        const Material*             material,
        const foundation::Vector3d& p,
        const foundation::Vector3d& x,
        const foundation::Vector3d& y,
        const foundation::Vector3d& n);

    ShapeType get_shape_type() const;

    const AssemblyInstance* get_assembly_instance() const;

    size_t get_object_instance_index() const;

    size_t get_primitive_index() const;

    float get_area() const;
    float get_rcp_area() const;

    float get_shape_prob() const;
    void set_shape_prob(const float prob);

    const Material* get_material() const;

    void sample_uniform(
        const foundation::Vector2f& s,
        const float                 shape_prob,
        LightSample&                light_sample) const;

    float evaluate_pdf_uniform() const;

    bool sample_solid_angle(
        const ShadingPoint&         shading_point,
        const foundation::Vector2f& s,
        const float                 shape_prob,
        LightSample&                light_sample) const;

    float evaluate_pdf_solid_angle(
        const foundation::Vector3d& p,
        const foundation::Vector3d& l) const;

    void make_shading_point(
        ShadingPoint&               shading_point,
        const foundation::Vector3d& point,
        const foundation::Vector3d& direction,
        const foundation::Vector2f& bary,
        const Intersector&          intersector) const;

    void estimate_average_radiance();
    float get_average_radiance() const;

  private:
    friend class LightSamplerBase;

    struct Disk
    {
        foundation::Vector3d    m_center;   // world space center of the disk
        foundation::Vector3d    m_normal;   // world space normal of the disk
        double                  m_radius;   // disk radius
        foundation::Matrix4d    m_object_to_world;
    };

    struct Triangle
    {
        foundation::Vector3d        m_v0, m_v1, m_v2;   // world space vertices of the shape
        foundation::Vector3d        m_n0, m_n1, m_n2;   // world space vertex normals
        foundation::Vector3d        m_geometric_normal; // world space geometric normal, unit-length
        double                      m_plane_dist;
    };

    struct Sphere
    {
        foundation::Vector3d    m_center;   // world space center of the sphere
        double                  m_radius;   // sphere radius
    };

    struct Rect
    {
        foundation::Vector3d        m_origin;           // world space position of the bottom left corner of the rect
        foundation::Vector3d        m_x, m_y;           // world space x and y axes
        double                      m_width;            // rectangle width
        double                      m_height;           // rectangle height
        foundation::Vector3d        m_geometric_normal; // world space geometric normal, unit-length
        double                      m_plane_dist;
    };

    union Geom
    {
        Disk     m_disk;
        Triangle m_triangle;
        Sphere   m_sphere;
        Rect     m_rect;
    };

    typedef foundation::stamped_ptr<const AssemblyInstance> AssemblyInstaceAndType;

    AssemblyInstaceAndType      m_assembly_instance_and_type;
    size_t                      m_object_instance_index;
    size_t                      m_primitive_index;
    Geom                        m_geom;
    TriangleSupportPlaneType    m_shape_support_plane;          // support plane of the shape in assembly space
    float                       m_area;                         // world space shape area
    float                       m_rcp_area;                     // world space shape area reciprocal
    float                       m_shape_prob;                   // probability density of this shape
    float                       m_average_radiance;             // estimated average radiance emitted from this shape
    const Material*             m_material;

    // Constructor.
    EmittingShape(
        const ShapeType             shape_type,
        const AssemblyInstance*     assembly_instance,
        const size_t                object_instance_index,
        const size_t                primitive_index,
        const Material*             material);
};


//
// EmittingShape class implementation.
//

inline EmittingShape::ShapeType EmittingShape::get_shape_type() const
{
    return static_cast<ShapeType>(m_assembly_instance_and_type.get_stamp());
}

inline const AssemblyInstance* EmittingShape::get_assembly_instance() const
{
    return m_assembly_instance_and_type.get_ptr();
}

inline size_t EmittingShape::get_primitive_index() const
{
    return m_primitive_index;
}

inline size_t EmittingShape::get_object_instance_index() const
{
    return m_object_instance_index;
}

inline float EmittingShape::get_area() const
{
    return m_area;
}

inline float EmittingShape::get_rcp_area() const
{
    return m_rcp_area;
}

inline float EmittingShape::get_shape_prob() const
{
    return m_shape_prob;
}

inline void EmittingShape::set_shape_prob(const float prob)
{
    m_shape_prob = prob;
}

inline const Material* EmittingShape::get_material() const
{
    return m_material;
}

inline float EmittingShape::get_average_radiance() const
{
    return m_average_radiance;
}

}       // namespace renderer
