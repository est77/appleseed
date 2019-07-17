
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
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

// Interface header.
#include "meshobjectoperations.h"

// appleseed.renderer headers.
#include "renderer/global/globaltypes.h"
#include "renderer/modeling/object/meshobject.h"
#include "renderer/modeling/object/triangle.h"
#include "renderer/utility/triangle.h"

// appleseed.foundation headers.
#include "foundation/array/arrayref.h"
#include "foundation/geometry/mesh.h"
#include "foundation/math/vector.h"
#include "foundation/utility/murmurhash.h"

// Standard headers.
#include <cassert>
#include <cstddef>
#include <vector>

using namespace foundation;
using namespace std;

namespace renderer
{

void compute_smooth_vertex_normals_base_pose(MeshObject& object)
{
    assert(object.get_vertex_normal_count() == 0);

    const size_t vertex_count = object.get_vertex_count();
    const size_t triangle_count = object.get_triangle_count();

    vector<GVector3> normals(vertex_count, GVector3(0.0));

    for (size_t i = 0; i < triangle_count; ++i)
    {
        Triangle& triangle = object.get_triangle(i);
        triangle.m_n0 = triangle.m_v0;
        triangle.m_n1 = triangle.m_v1;
        triangle.m_n2 = triangle.m_v2;

        const GVector3& v0 = object.get_vertex(triangle.m_v0);
        const GVector3& v1 = object.get_vertex(triangle.m_v1);
        const GVector3& v2 = object.get_vertex(triangle.m_v2);
        const GVector3 normal = normalize(compute_triangle_normal(v0, v1, v2));

        normals[triangle.m_v0] += normal;
        normals[triangle.m_v1] += normal;
        normals[triangle.m_v2] += normal;
    }

    object.reserve_vertex_normals(vertex_count);

    for (size_t i = 0; i < vertex_count; ++i)
        object.push_vertex_normal(safe_normalize(normals[i]));
}

void compute_smooth_vertex_normals_pose(MeshObject& object, const size_t motion_segment_index)
{
    const size_t vertex_count = object.get_vertex_count();
    const size_t triangle_count = object.get_triangle_count();

    vector<GVector3> normals(vertex_count, GVector3(0.0));

    for (size_t i = 0; i < triangle_count; ++i)
    {
        const Triangle& triangle = object.get_triangle(i);

        const GVector3& v0 = object.get_vertex_pose(triangle.m_v0, motion_segment_index);
        const GVector3& v1 = object.get_vertex_pose(triangle.m_v1, motion_segment_index);
        const GVector3& v2 = object.get_vertex_pose(triangle.m_v2, motion_segment_index);
        const GVector3 normal = normalize(compute_triangle_normal(v0, v1, v2));

        normals[triangle.m_v0] += normal;
        normals[triangle.m_v1] += normal;
        normals[triangle.m_v2] += normal;
    }

    for (size_t i = 0; i < vertex_count; ++i)
        object.set_vertex_normal_pose(i, motion_segment_index, safe_normalize(normals[i]));
}

void compute_smooth_vertex_normals(MeshObject& object)
{
    compute_smooth_vertex_normals_base_pose(object);

    for (size_t i = 0; i < object.get_motion_segment_count(); ++i)
        compute_smooth_vertex_normals_pose(object, i);
}

void compute_smooth_vertex_tangents_base_pose(MeshObject& object)
{
    assert(object.get_vertex_tangent_count() == 0);
    assert(object.get_tex_coords_count() > 0);

    const size_t vertex_count = object.get_vertex_count();
    const size_t triangle_count = object.get_triangle_count();

    vector<GVector3> tangents(vertex_count, GVector3(0.0));

    for (size_t i = 0; i < triangle_count; ++i)
    {
        const Triangle& triangle = object.get_triangle(i);

        if (!triangle.has_vertex_attributes())
            continue;

        const GVector2 v0_uv = object.get_tex_coords(triangle.m_a0);
        const GVector2 v1_uv = object.get_tex_coords(triangle.m_a1);
        const GVector2 v2_uv = object.get_tex_coords(triangle.m_a2);

        //
        // Reference:
        //
        //   Physically Based Rendering, first edition, pp. 128-129
        //

        const GScalar du0 = v0_uv[0] - v2_uv[0];
        const GScalar dv0 = v0_uv[1] - v2_uv[1];
        const GScalar du1 = v1_uv[0] - v2_uv[0];
        const GScalar dv1 = v1_uv[1] - v2_uv[1];
        const GScalar det = du0 * dv1 - dv0 * du1;

        if (det == GScalar(0.0))
            continue;

        const GVector3& v2 = object.get_vertex(triangle.m_v2);
        const GVector3 dp0 = object.get_vertex(triangle.m_v0) - v2;
        const GVector3 dp1 = object.get_vertex(triangle.m_v1) - v2;
        const GVector3 tangent = normalize(dv1 * dp0 - dv0 * dp1);

        tangents[triangle.m_v0] += tangent;
        tangents[triangle.m_v1] += tangent;
        tangents[triangle.m_v2] += tangent;
    }

    object.reserve_vertex_tangents(vertex_count);

    for (size_t i = 0; i < vertex_count; ++i)
        object.push_vertex_tangent(safe_normalize(tangents[i]));
}

void compute_smooth_vertex_tangents_pose(MeshObject& object, const size_t motion_segment_index)
{
    assert(object.get_tex_coords_count() > 0);

    const size_t vertex_count = object.get_vertex_count();
    const size_t triangle_count = object.get_triangle_count();

    vector<GVector3> tangents(vertex_count, GVector3(0.0));

    for (size_t i = 0; i < triangle_count; ++i)
    {
        const Triangle& triangle = object.get_triangle(i);

        if (!triangle.has_vertex_attributes())
            continue;

        const GVector2 v0_uv = object.get_tex_coords(triangle.m_a0);
        const GVector2 v1_uv = object.get_tex_coords(triangle.m_a1);
        const GVector2 v2_uv = object.get_tex_coords(triangle.m_a2);

        //
        // Reference:
        //
        //   Physically Based Rendering, first edition, pp. 128-129
        //

        const GScalar du0 = v0_uv[0] - v2_uv[0];
        const GScalar dv0 = v0_uv[1] - v2_uv[1];
        const GScalar du1 = v1_uv[0] - v2_uv[0];
        const GScalar dv1 = v1_uv[1] - v2_uv[1];
        const GScalar det = du0 * dv1 - dv0 * du1;

        if (det == GScalar(0.0))
            continue;

        const GVector3& v2 = object.get_vertex_pose(triangle.m_v2, motion_segment_index);
        const GVector3 dp0 = object.get_vertex_pose(triangle.m_v0, motion_segment_index) - v2;
        const GVector3 dp1 = object.get_vertex_pose(triangle.m_v1, motion_segment_index) - v2;
        const GVector3 tangent = normalize(dv1 * dp0 - dv0 * dp1);

        tangents[triangle.m_v0] += tangent;
        tangents[triangle.m_v1] += tangent;
        tangents[triangle.m_v2] += tangent;
    }

    for (size_t i = 0; i < vertex_count; ++i)
        object.set_vertex_tangent_pose(i, motion_segment_index, safe_normalize(tangents[i]));
}

void compute_smooth_vertex_tangents(MeshObject& object)
{
    compute_smooth_vertex_tangents_base_pose(object);

    for (size_t i = 0; i < object.get_motion_segment_count(); ++i)
        compute_smooth_vertex_tangents_pose(object, i);
}

void compute_signature(MurmurHash& hash, const MeshObject& object)
{
    // Static attributes.

    hash.append(object.get_triangle_count());
    for (size_t i = 0, e = object.get_triangle_count(); i < e; ++i)
        hash.append(object.get_triangle(i));

    hash.append(object.get_material_slot_count());
    for (size_t i = 0, e = object.get_material_slot_count(); i < e; ++i)
        hash.append(object.get_material_slot(i));

    hash.append(object.get_vertex_count());
    for (size_t i = 0, e = object.get_vertex_count(); i < e; ++i)
        hash.append(object.get_vertex(i));

    hash.append(object.get_tex_coords_count());
    for (size_t i = 0, e = object.get_tex_coords_count(); i < e; ++i)
        hash.append(object.get_tex_coords(i));

    hash.append(object.get_vertex_normal_count());
    for (size_t i = 0, e = object.get_vertex_normal_count(); i < e; ++i)
        hash.append(object.get_vertex_normal(i));

    hash.append(object.get_vertex_tangent_count());
    for (size_t i = 0, e = object.get_vertex_tangent_count(); i < e; ++i)
        hash.append(object.get_vertex_tangent(i));

    // Poses.

    hash.append(object.get_motion_segment_count());
    for (size_t j = 0, je = object.get_motion_segment_count(); j < je; ++j)
    {
        for (size_t i = 0, e = object.get_vertex_count(); i < e; ++i)
            hash.append(object.get_vertex_pose(i, j));

        for (size_t i = 0, e = object.get_vertex_normal_count(); i < e; ++i)
            hash.append(object.get_vertex_normal_pose(i, j));

        for (size_t i = 0, e = object.get_vertex_tangent_count(); i < e; ++i)
            hash.append(object.get_vertex_tangent_pose(i, j));
    }
}

Mesh mesh2mesh(const MeshObject& object)
{
    Mesh mesh;

    const size_t num_faces = object.get_triangle_count();
    const size_t num_vertices = object.get_vertex_count();
    const size_t num_uvs = object.get_tex_coords_count();
    const size_t num_normals = object.get_vertex_normal_count();
    const size_t num_tangents = object.get_vertex_tangent_count();

    const size_t num_keys = object.get_motion_segment_count();

    // Vertices per face.
    ArrayRef<uint32> nverts(mesh.get_verts_per_face().write());
    nverts.fill(num_faces, 3);

    // Vertex indices.
    ArrayRef<uint32> vindx(mesh.get_vertex_indices().write());
    vindx.reserve(num_faces * 3);

    for (size_t i = 0; i < num_faces; ++i)
    {
        const Triangle& tri = object.get_triangle(i);
        vindx.push_back(tri.m_v0);
        vindx.push_back(tri.m_v1);
        vindx.push_back(tri.m_v2);
    }

    // Vertices.
    mesh.get_vertices().write().resize(num_vertices, num_keys);
    {
        ArrayRef<Vector3f> P(mesh.get_vertices().write().get_key(0));
        for (size_t i = 0; i < num_vertices; ++i)
            P[i] = object.get_vertex(i);
    }

    for (size_t j = 1; j < num_keys; ++j)
    {
        ArrayRef<Vector3f> P(mesh.get_vertices().write().get_key(j));
        for (size_t i = 0; i < num_vertices; ++i)
            P[i] = object.get_vertex_pose(i, j);
    }

    if (num_uvs != 0)
    {
        // UV indices.
        ArrayRef<uint32> uvindx(mesh.get_uv_indices().write());
        uvindx.reserve(num_faces * 3);

        for (size_t i = 0; i < num_faces; ++i)
        {
            const Triangle& tri = object.get_triangle(i);
            vindx.push_back(tri.m_a0);
            vindx.push_back(tri.m_a1);
            vindx.push_back(tri.m_a2);
        }

        // UVs.
        ArrayRef<Vector2f> uvs(mesh.get_uvs().write());
        uvs.reserve(num_uvs);

        for (size_t i = 0; i < num_uvs; ++i)
            uvs.push_back(object.get_tex_coords(i));
    }

    if (num_normals != 0)
    {
        // Normal indices.
        ArrayRef<uint32> nindx(mesh.get_normal_indices().write());
        nindx.reserve(num_faces * 3);

        for (size_t i = 0; i < num_faces; ++i)
        {
            const Triangle& tri = object.get_triangle(i);
            vindx.push_back(tri.m_n0);
            vindx.push_back(tri.m_n1);
            vindx.push_back(tri.m_n2);
        }

        // Normals.
        mesh.get_normals().write().resize(num_normals, num_keys);
        {
            ArrayRef<Vector3f> N(mesh.get_normals().write().get_key(0));
            for (size_t i = 0; i < num_normals; ++i)
                N[i] = object.get_vertex_normal(i);
        }

        for (size_t j = 1; j < num_keys; ++j)
        {
            ArrayRef<Vector3f> N(mesh.get_normals().write().get_key(j));
            for (size_t i = 1; i < num_normals; ++i)
                N[i] = object.get_vertex_normal_pose(i, j);
        }
    }

    // Tangents.
    if (num_tangents != 0)
    {
        // Tangent indices.
        mesh.get_tangent_indices().write() = mesh.get_normal_indices().read();

        // Tangents.
        mesh.get_tangents().write().resize(num_tangents, num_keys);
        {
            ArrayRef<Vector3f> T(mesh.get_tangents().write().get_key(0));
            for (size_t i = 0; i < num_tangents; ++i)
                T[i] = object.get_vertex_tangent(i);
        }

        for (size_t j = 1; j < num_keys; ++j)
        {
            ArrayRef<Vector3f> T(mesh.get_tangents().write().get_key(j));
            for (size_t i = 1; i < num_tangents; ++i)
                T[i] = object.get_vertex_tangent_pose(i, j);
        }
    }

    // Material indices.
    ArrayRef<uint32> matindx(mesh.get_material_indices().write());
    matindx.reserve(num_faces * 3);

    for (size_t i = 0; i < num_faces; ++i)
        matindx.push_back(object.get_triangle(i).m_pa);

    return mesh;
}

}   // namespace renderer
