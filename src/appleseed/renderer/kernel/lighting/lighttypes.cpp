
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2018 Esteban Tovagliari, The appleseedhq Organization
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
#include "lighttypes.h"

// appleseed.renderer headers.
#include "renderer/kernel/intersection/intersector.h"
#include "renderer/kernel/lighting/lightsample.h"

// appleseed.foundation headers.
#include "foundation/math/basis.h"
#include "foundation/math/distance.h"
#include "foundation/math/intersection/rayparallelogram.h"
#include "foundation/math/intersection/raytrianglemt.h"
#include "foundation/math/sampling/mappings.h"
#include "foundation/math/sampling/sphericalrectanglesampler.h"
#include "foundation/math/sampling/sphericaltrianglesampler.h"

using namespace foundation;

namespace renderer
{

//
// EmittingShape class implementation.
//
// References:
//
//   [1] Monte Carlo Techniques for Direct Lighting Calculations.
//       http://www.cs.virginia.edu/~jdl/bib/globillum/mis/shirley96.pdf
//
//   [2] Stratified Sampling of Spherical Triangles.
//       https://www.graphics.cornell.edu/pubs/1995/Arv95c.pdf
//
//   [3] An Area-Preserving Parametrization for Spherical Rectangles.
//       https://www.arnoldrenderer.com/research/egsr2013_spherical_rectangle.pdf
//
//

namespace
{

template <typename Shape>
double signed_plane_distance(const Shape& shape, const Vector3d& p)
{
    return dot(p, shape.m_geometric_normal) + shape.m_plane_dist;
}

}

EmittingShape EmittingShape::create_triangle_shape(
    const AssemblyInstance*     assembly_instance,
    const size_t                object_instance_index,
    const size_t                primitive_index,
    const Material*             material,
    const Vector3d&             v0,
    const Vector3d&             v1,
    const Vector3d&             v2,
    const Vector3d&             n0,
    const Vector3d&             n1,
    const Vector3d&             n2,
    const Vector3d&             geometric_normal)
{
    EmittingShape shape(
        TriangleShape,
        assembly_instance,
        object_instance_index,
        primitive_index,
        material);

    shape.m_geom.m_triangle.m_v0 = v0;
    shape.m_geom.m_triangle.m_v1 = v1;
    shape.m_geom.m_triangle.m_v2 = v2;
    shape.m_geom.m_triangle.m_n0 = n0;
    shape.m_geom.m_triangle.m_n1 = n1;
    shape.m_geom.m_triangle.m_n2 = n2;
    shape.m_geom.m_triangle.m_geometric_normal = geometric_normal;
    shape.m_geom.m_triangle.m_plane_dist = -dot(v0, geometric_normal);

    return shape;
}

EmittingShape EmittingShape::create_sphere_shape(
    const AssemblyInstance*     assembly_instance,
    const size_t                object_instance_index,
    const Material*             material,
    const Vector3d&             center,
    const double                radius)
{
    EmittingShape shape(
        SphereShape,
        assembly_instance,
        object_instance_index,
        0,
        material);

    shape.m_geom.m_sphere.m_center = center;
    shape.m_geom.m_sphere.m_radius = radius;

    shape.m_area = static_cast<float>(FourPi<double>() * square(radius));

    if (shape.m_area != 0.0f)
        shape.m_rcp_area = 1.0f / shape.m_area;

    return shape;
}

EmittingShape EmittingShape::create_rect_shape(
    const AssemblyInstance*     assembly_instance,
    const size_t                object_instance_index,
    const Material*             material,
    const Vector3d&             p,
    const Vector3d&             x,
    const Vector3d&             y,
    const Vector3d&             n)
{
    EmittingShape shape(
        RectShape,
        assembly_instance,
        object_instance_index,
        0,
        material);

    shape.m_geom.m_rect.m_origin = p;
    shape.m_geom.m_rect.m_x = x;
    shape.m_geom.m_rect.m_y = y;
    shape.m_geom.m_rect.m_width = norm(x);
    shape.m_geom.m_rect.m_height = norm(y);
    shape.m_geom.m_rect.m_geometric_normal = n;
    shape.m_geom.m_rect.m_plane_dist = -dot(p, n);

    shape.m_area = static_cast<float>(
        shape.m_geom.m_rect.m_width * shape.m_geom.m_rect.m_height);

    if (shape.m_area != 0.0f)
        shape.m_rcp_area = 1.0f / shape.m_area;

    return shape;
}

EmittingShape::EmittingShape(
    const ShapeType         shape_type,
    const AssemblyInstance* assembly_instance,
    const size_t            object_instance_index,
    const size_t            primitive_index,
    const Material*         material)
{
    m_assembly_instance_and_type.set(
        assembly_instance,
        static_cast<foundation::uint16>(shape_type));

    m_object_instance_index = object_instance_index;
    m_primitive_index = primitive_index;
    m_material = material;
    m_shape_prob = 0.0f;
    m_average_radiance = 1.0f;
}

void EmittingShape::sample_uniform(
    const Vector2f&         s,
    const float             shape_prob,
    LightSample&            light_sample) const
{
    // Store a pointer to the emitting shape.
    light_sample.m_shape = this;

    const auto shape_type = get_shape_type();

    if (shape_type == TriangleShape)
    {
        // Uniformly sample the surface of the shape.
        const Vector3d bary = sample_triangle_uniform(Vector2d(s));

        // Set the barycentric coordinates.
        light_sample.m_bary[0] = static_cast<float>(bary[0]);
        light_sample.m_bary[1] = static_cast<float>(bary[1]);

        // Compute the world space position of the sample.
        light_sample.m_point =
              bary[0] * m_geom.m_triangle.m_v0
            + bary[1] * m_geom.m_triangle.m_v1
            + bary[2] * m_geom.m_triangle.m_v2;

        // Compute the world space shading normal at the position of the sample.
        light_sample.m_shading_normal =
              bary[0] * m_geom.m_triangle.m_n0
            + bary[1] * m_geom.m_triangle.m_n1
            + bary[2] * m_geom.m_triangle.m_n2;
        light_sample.m_shading_normal = normalize(light_sample.m_shading_normal);

        // Set the world space geometric normal.
        light_sample.m_geometric_normal = m_geom.m_triangle.m_geometric_normal;
    }
    else if (shape_type == SphereShape)
    {
        // Set the barycentric coordinates.
        light_sample.m_bary = s;

        Vector3d n(sample_sphere_uniform(s));

        // Set the world space shading and geometric normal.
        light_sample.m_shading_normal = n;
        light_sample.m_geometric_normal = n;

        // Compute the world space position of the sample.
        light_sample.m_point =
            m_geom.m_sphere.m_center + n * m_geom.m_sphere.m_radius;
    }
    else if (shape_type == RectShape)
    {
        // Set the barycentric coordinates.
        light_sample.m_bary = s;

        light_sample.m_point =
            m_geom.m_rect.m_origin +
            static_cast<double>(s[0]) * m_geom.m_rect.m_x +
            static_cast<double>(s[1]) * m_geom.m_rect.m_y;

        // Set the world space shading and geometric normal.
        light_sample.m_shading_normal = m_geom.m_rect.m_geometric_normal;
        light_sample.m_geometric_normal = m_geom.m_rect.m_geometric_normal;
    }
    else
    {
        assert(false && "Unknown emitter shape type");
    }

    // Compute the probability density of this sample.
    light_sample.m_probability = shape_prob * get_rcp_area();
}

float EmittingShape::evaluate_pdf_uniform() const
{
    return get_shape_prob() * get_rcp_area();
}

// Comment to disable solid angle sampling.
#define USE_SOLID_ANGLE_SAMPLING

bool EmittingShape::sample_solid_angle(
    const ShadingPoint&     shading_point,
    const Vector2f&         s,
    const float             shape_prob,
    LightSample&            light_sample) const
{
#ifdef USE_SOLID_ANGLE_SAMPLING
    // Store a pointer to the emitting shape.
    light_sample.m_shape = this;

    const auto shape_type = get_shape_type();

    if (shape_type == TriangleShape)
    {
        const Vector3d o = shading_point.get_point();

        // Side check
        const double eps = 1.0e-6;
        if (signed_plane_distance(m_geom.m_triangle, o) < eps)
            return false;

        const SphericalTriangleSampler<double> sampler(
            m_geom.m_triangle.m_v0,
            m_geom.m_triangle.m_v1,
            m_geom.m_triangle.m_v2,
            o);

        const Vector3d d = sampler.sample(Vector2d(s));

        // Project the point on the triangle.
        const Ray<double, 3> ray(o, d);
        TriangleMT<double> triangle(
            m_geom.m_triangle.m_v0,
            m_geom.m_triangle.m_v1,
            m_geom.m_triangle.m_v2);

        double t, u, v;
        if (triangle.intersect(ray, t, u, v))
        {
            light_sample.m_point = o + t * d;
            light_sample.m_bary[0] = static_cast<float>(u);
            light_sample.m_bary[1] = static_cast<float>(v);
            light_sample.m_geometric_normal = m_geom.m_triangle.m_geometric_normal;

            const Vector3d n =
              m_geom.m_triangle.m_n0 * (1.0 - u - v)
            + m_geom.m_triangle.m_n1 * u
            + m_geom.m_triangle.m_n2 * v;
            light_sample.m_shading_normal = normalize(n);

            // Compute the probability.
            const double cos_theta = -dot(m_geom.m_triangle.m_geometric_normal, d);
            const double rcp_solid_angle = 1.0 / sampler.solid_angle();

            const double pdf = rcp_solid_angle * cos_theta / square(t);
            light_sample.m_probability = shape_prob * static_cast<float>(pdf);
            return true;
        }
    }
    else if (shape_type == SphereShape)
    {
        // Source:
        // https://schuttejoe.github.io/post/arealightsampling/
        const Vector3d& center  = m_geom.m_sphere.m_center;
        const Vector3d& origin  = shading_point.get_point();
        const double    radius  = m_geom.m_sphere.m_radius;

        Vector3d        w               = center - origin;
        const double    dist_to_center  = norm(w);

        // Normalize center to origin vector.
        w *= 1.0 / dist_to_center;

        // Create a orthogonal frame that simplifies the projection.
        const Basis3d frame(w);
        const Vector3d& u = frame.get_tangent_u();
        const Vector3d& v = frame.get_tangent_v();

        // Compute the matrix groing from local to world space.
        Matrix3d world;
        world[0] = u[0];
        world[1] = u[1];
        world[2] = u[2];
        world[3] = w[0];
        world[4] = w[1];
        world[5] = w[2];
        world[6] = v[0];
        world[7] = v[1];
        world[8] = v[2];

        // Compute local space sample position.
        const double q = sqrt(1.0 - square(radius / dist_to_center));
        const double    theta   = acos(1.0 - static_cast<double>(s[0]) + static_cast<double>(s[0]) * q);
        const double    phi     = TwoPi<double>() * static_cast<double>(s[1]);
        const Vector3d  local   = Vector3d::make_unit_vector(theta, phi);

        // Compute world space sample position.
        {
            const Vector3d nwp = local * world;
            const Vector3d x = origin - center;

            const double b = 2.0 * dot(nwp, x);
            const double c = dot(x, x) - radius * radius;

            double t;

            const double root = b * b - 4.0 * c;
            if(root < 0.0)
            {
                // Project x onto v.
                const Vector3d projected_x = (dot(x, nwp) / dot(nwp, nwp)) * nwp;
                t = norm(projected_x);
            }
            else if(root == 0.0)
            {
                t = -0.5 * b;
            }
            else
            {
                const double q = (b > 0.0) ? -0.5 * (b + sqrt(root)) : -0.5 * (b - sqrt(root));
                const double t0 = q;
                const double t1 = c / q;
                t = min(t0, t1);
            }

            light_sample.m_point = origin + t * nwp;
        }

        // Compute the normal at the sample.
        light_sample.m_shading_normal = normalize(
            light_sample.m_point - m_geom.m_sphere.m_center);
        light_sample.m_geometric_normal = light_sample.m_shading_normal;        
        light_sample.m_bary[0] = static_cast<double>(theta);
        light_sample.m_bary[1] = static_cast<double>(phi);

        // Compute the probability.
        const float pdf = 1.0f / (TwoPi<float>() * (1.0f - static_cast<float>(q)));
        light_sample.m_probability = shape_prob * pdf;
        return true;
    }
    else if (shape_type == RectShape)
    {
        const Vector3d o = shading_point.get_point();

        // Side check
        const double eps = 1.0e-6;
        if (signed_plane_distance(m_geom.m_rect, o) < eps)
            return false;

        SphericalRectangleSampler<double> sampler(
            m_geom.m_rect.m_origin,
            m_geom.m_rect.m_x,
            m_geom.m_rect.m_y,
            m_geom.m_rect.m_geometric_normal,
            o);

        const Vector3d p = sampler.sample(Vector2d(s));
        const Vector3d d = normalize(p - o);

        // Project the point on the rect.
        const Ray<double, 3> ray(o, d);

        double t, u, v;
        if (intersect_parallelogram(
                ray,
                m_geom.m_rect.m_origin,
                m_geom.m_rect.m_x,
                m_geom.m_rect.m_y,
                m_geom.m_rect.m_geometric_normal,
                t,
                u,
                v))
        {
            light_sample.m_point = ray.point_at(t);
            light_sample.m_bary[0] = static_cast<float>(u);
            light_sample.m_bary[1] = static_cast<float>(v);
            light_sample.m_geometric_normal = m_geom.m_rect.m_geometric_normal;
            light_sample.m_shading_normal = light_sample.m_geometric_normal;

            // Compute the probability.
            const double cos_theta = -dot(m_geom.m_rect.m_geometric_normal, d);
            const double rcp_solid_angle = 1.0 / sampler.solid_angle();

            const double pdf = rcp_solid_angle * cos_theta / t;
            light_sample.m_probability = shape_prob * static_cast<float>(pdf);
            return true;
        }
    }
    else
    {
        assert(false && "Unknown emitter shape type");
    }

    return false;

#else
    sample_uniform(s, shape_prob, light_sample);
    return true;
#endif
}

float EmittingShape::evaluate_pdf_solid_angle(
    const Vector3d&         p,
    const Vector3d&         l) const
{
#ifdef USE_SOLID_ANGLE_SAMPLING
    const auto shape_type = get_shape_type();

    const float shape_probability = get_shape_prob();

    if (shape_type == TriangleShape)
    {
        // Side check
        const double eps = 1.0e-6;
        if (signed_plane_distance(m_geom.m_triangle, p) < eps)
            return 0.0f;

        const SphericalTriangleSampler<double> sampler(
            m_geom.m_triangle.m_v0,
            m_geom.m_triangle.m_v1,
            m_geom.m_triangle.m_v2,
            p);

        Vector3d d = p - l;
        const double d_norm = norm(d);
        d /= d_norm;

        const double cos_theta = dot(m_geom.m_triangle.m_geometric_normal, d);
        const double rcp_solid_angle = 1.0 / sampler.solid_angle();

        const double pdf = rcp_solid_angle * cos_theta / square(d_norm);
        return shape_probability * static_cast<float>(pdf);
    }
    else if (shape_type == SphereShape)
    {
        /*
        Vector3d w = m_geom.m_triangle.m_v0 - p;
        const double dist_to_center = norm(w);
        w *= 1.0 / dist_to_center;
        const double q = sqrt(1.0 - square(m_geom.m_triangle.m_v1.x / dist_to_center));
        const float pdf = 1.0f / (TwoPi<float>() * (1.0f - static_cast<float>(q)));
        return shape_probability * pdf;
        */
        const Vector3d& center = m_geom.m_sphere.m_center;
        const double radius_sqr = square(m_geom.m_sphere.m_radius);

        const double dist_to_center = square_distance(p, center);
        if (dist_to_center <= radius_sqr)
            return FourPi<float>();

        const float sin_theta_sqr = static_cast<float>(radius_sqr / dist_to_center);
        const float cos_theta = sqrt(max(0.0f, 1.0f - sin_theta_sqr));

        return shape_probability * TwoPi<float>() * (1.0f - cos_theta);
    }
    else if (shape_type == RectShape)
    {
        // Side check
        const double eps = 1.0e-6;
        if (signed_plane_distance(m_geom.m_rect, p) < eps)
            return false;

        SphericalRectangleSampler<double> sampler(
            m_geom.m_rect.m_origin,
            m_geom.m_rect.m_x,
            m_geom.m_rect.m_y,
            m_geom.m_rect.m_geometric_normal,
            p);

        Vector3d d = p - l;
        const double d_norm = norm(d);
        d /= d_norm;

        const double cos_theta = dot(m_geom.m_rect.m_geometric_normal, d);
        const double rcp_solid_angle = 1.0 / sampler.solid_angle();

        const double pdf = rcp_solid_angle * cos_theta / square(d_norm);
        return shape_probability * static_cast<float>(pdf);
    }
    else
    {
        assert(false && "Unknown emitter shape type");
        return -1.0f;
    }
#else
    return evaluate_pdf_uniform();
#endif
}

void EmittingShape::make_shading_point(
    ShadingPoint&           shading_point,
    const Vector3d&         point,
    const Vector3d&         direction,
    const Vector2f&         bary,
    const Intersector&      intersector) const
{
    const ShadingRay ray(
        point,
        direction,
        0.0,
        0.0,
        ShadingRay::Time(),
        VisibilityFlags::CameraRay, 0);

    const auto shape_type = get_shape_type();

    if (shape_type == TriangleShape)
    {
        intersector.make_triangle_shading_point(
            shading_point,
            ray,
            bary,
            get_assembly_instance(),
            get_assembly_instance()->transform_sequence().get_earliest_transform(),
            get_object_instance_index(),
            get_primitive_index(),
            m_shape_support_plane);
    }
    else if (shape_type == SphereShape)
    {
        const double theta = static_cast<double>(bary[0]);
        const double phi   = static_cast<double>(bary[1]);

        const Vector3d n = Vector3d::make_unit_vector(theta, phi);
        const Vector3d p = m_geom.m_sphere.m_center + m_geom.m_sphere.m_radius * n;

        const Vector3d dpdu(-TwoPi<double>() * n.y, TwoPi<double>() + n.x, 0.0);
        const Vector3d dpdv = cross(dpdu, n);

        intersector.make_procedural_surface_shading_point(
            shading_point,
            ray,
            bary,
            get_assembly_instance(),
            get_assembly_instance()->transform_sequence().get_earliest_transform(),
            get_object_instance_index(),
            get_primitive_index(),
            p,
            n,
            dpdu,
            dpdv);
    }
    else if (shape_type == RectShape)
    {
        const Vector3d p =
            m_geom.m_rect.m_origin +
            static_cast<double>(bary[0]) * m_geom.m_rect.m_x +
            static_cast<double>(bary[1]) * m_geom.m_rect.m_y;

        intersector.make_procedural_surface_shading_point(
            shading_point,
            ray,
            bary,
            get_assembly_instance(),
            get_assembly_instance()->transform_sequence().get_earliest_transform(),
            get_object_instance_index(),
            get_primitive_index(),
            p,
            m_geom.m_rect.m_geometric_normal,
            m_geom.m_rect.m_x,
            cross(m_geom.m_rect.m_x, m_geom.m_rect.m_geometric_normal));
    }
    else
    {
        assert(false && "Unknown emitter shape type");
    }
}

void EmittingShape::estimate_average_radiance()
{
    // todo:
    /*
    if (constant EDF)
        return EDF->radiance();

    // Varying EDF or OSL emission case.
    for i = 0..N:
    {
        s = random2d()
        make_shading_point(shading_point, p, d, s, intersector);
        radiance += eval EDF or ShaderGroup
    }

    radiance /= N;
    return radiance;
    */

    m_average_radiance = 1.0f;
}

}       // namespace renderer
