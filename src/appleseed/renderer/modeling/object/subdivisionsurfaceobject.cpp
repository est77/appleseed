
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
#include "subdivisionsurfaceobject.h"

// appleseed.foundation headers.
#include "foundation/utility/makevector.h"

// Standard headers.
#include <cassert>
#include <string>
#include <vector>

using namespace foundation;
using namespace std;

namespace renderer
{

//
// SubdivisionSurfaceObject class implementation.
//

struct SubdivisionSurfaceObject::Impl
{
    typedef std::vector<GVector3> Vector3Array;
    typedef std::vector<GVector2> Vector2Array;
    typedef std::vector<uint32>   IndexArray;

    RegionKit               m_region_kit;
    Lazy<RegionKit>         m_lazy_region_kit;
    Vector3Array            m_vertices;
    IndexArray              m_edges_per_face;
    IndexArray              m_face_vertices;
    Vector2Array            m_uvs;
    vector<string>          m_material_slots;
    SubdivisionScheme       m_subdivision_scheme;

    Impl()
      : m_lazy_region_kit(&m_region_kit)
    {
    }

    GAABB3 compute_bounds() const
    {
        GAABB3 bbox;
        bbox.invalidate();

        return bbox;
    }
};

SubdivisionSurfaceObject::SubdivisionSurfaceObject(
    const char*         name,
    const ParamArray&   params)
  : Object(name, params)
  , impl(new Impl())
{
    m_inputs.declare("alpha_map", InputFormatScalar, "");

    const EntityDefMessageContext message_context("object", this);

    // Retrieve subdivision_scheme.
    const string subdivision_scheme =
        params.get_optional<string>(
            "subdivision_scheme",
            "catmull-clark",
            make_vector("bilinear", "catmull-clark", "loop"),
            message_context);
    if (subdivision_scheme == "bilinear")
        impl->m_subdivision_scheme = SubdivisionSchemeBilinear;
    else if (subdivision_scheme == "loop")
        impl->m_subdivision_scheme = SubdivisionSchemeLoop;
    impl->m_subdivision_scheme = SubdivisionSchemeCatmullClark;
}

SubdivisionSurfaceObject::~SubdivisionSurfaceObject()
{
    delete impl;
}

void SubdivisionSurfaceObject::release()
{
    delete this;
}

const char* SubdivisionSurfaceObject::get_model() const
{
    return SubdivisionSurfaceObjectFactory::get_model();
}

GAABB3 SubdivisionSurfaceObject::compute_local_bbox() const
{
    return impl->compute_bounds();
}

Lazy<RegionKit>& SubdivisionSurfaceObject::get_region_kit()
{
    return impl->m_lazy_region_kit;
}

void SubdivisionSurfaceObject::reserve_vertices(const size_t count)
{
    impl->m_vertices.reserve(count);
}

size_t SubdivisionSurfaceObject::push_vertex(const GVector3& vertex)
{
    const size_t index = impl->m_vertices.size();
    impl->m_vertices.push_back(vertex);
    return index;
}

size_t SubdivisionSurfaceObject::get_vertex_count() const
{
    return impl->m_vertices.size();
}

const GVector3& SubdivisionSurfaceObject::get_vertex(const size_t index) const
{
    assert(index < get_vertex_count());

    return impl->m_vertices[index];
}

void SubdivisionSurfaceObject::reserve_faces(const size_t count)
{
    impl->m_edges_per_face.reserve(count);
}

size_t SubdivisionSurfaceObject::push_face_edge_count(const size_t num_edges)
{
    assert(num_edges > 2);

    const size_t index = impl->m_edges_per_face.size();
    impl->m_edges_per_face.push_back(num_edges);
    return index;
}

size_t SubdivisionSurfaceObject::get_face_count() const
{
    return impl->m_edges_per_face.size();
}

void SubdivisionSurfaceObject::clear_faces()
{
   impl->m_edges_per_face.clear();
   impl->m_face_vertices.clear();
}

void SubdivisionSurfaceObject::reserve_edges(const size_t count)
{
    impl->m_face_vertices.reserve(count);
}

size_t SubdivisionSurfaceObject::get_face_num_edges(const size_t index) const
{
    assert(index < get_face_count());

    return impl->m_edges_per_face[index];
}

size_t SubdivisionSurfaceObject::push_face_vertex(const size_t vertex_index)
{
    const size_t index = impl->m_face_vertices.size();
    impl->m_face_vertices.push_back(vertex_index);
    return index;
}

void SubdivisionSurfaceObject::reserve_tex_coords(const size_t count)
{
    impl->m_uvs.reserve(count);
}

size_t SubdivisionSurfaceObject::push_tex_coords(const GVector2& tex_coords)
{
    const size_t index = impl->m_uvs.size();
    impl->m_uvs.push_back(tex_coords);
    return index;
}

size_t SubdivisionSurfaceObject::get_tex_coords_count() const
{
    return impl->m_uvs.size();
}

GVector2 SubdivisionSurfaceObject::get_tex_coords(const size_t index) const
{
    return impl->m_uvs[index];
}

void SubdivisionSurfaceObject::reserve_material_slots(const size_t count)
{
    impl->m_material_slots.reserve(count);
}

size_t SubdivisionSurfaceObject::push_material_slot(const char* name)
{
    const size_t index = impl->m_material_slots.size();
    impl->m_material_slots.push_back(name);
    return index;
}

size_t SubdivisionSurfaceObject::get_material_slot_count() const
{
    return impl->m_material_slots.size();
}

const char* SubdivisionSurfaceObject::get_material_slot(const size_t index) const
{
    return impl->m_material_slots[index].c_str();
}


//
// SubdivisionSurfaceObjectFactory class implementation.
//

const char* SubdivisionSurfaceObjectFactory::get_model()
{
    return "subdivmesh_object";
}

auto_release_ptr<SubdivisionSurfaceObject> SubdivisionSurfaceObjectFactory::create(
    const char*         name,
    const ParamArray&   params)
{
    return
        auto_release_ptr<SubdivisionSurfaceObject>(
            new SubdivisionSurfaceObject(name, params));
}

}   // namespace renderer

