
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

#ifndef APPLESEED_RENDERER_MODELING_OBJECT_SUBDIVISIONSURFACEOBJECT_H
#define APPLESEED_RENDERER_MODELING_OBJECT_SUBDIVISIONSURFACEOBJECT_H

// appleseed.renderer headers.
#include "renderer/global/globaltypes.h"
#include "renderer/kernel/intersection/intersectionsettings.h"
#include "renderer/modeling/object/object.h"
#include "renderer/modeling/object/regionkit.h"

// appleseed.foundation headers.
#include "foundation/platform/compiler.h"
#include "foundation/platform/types.h"
#include "foundation/utility/autoreleaseptr.h"
#include "foundation/utility/lazy.h"

// appleseed.main headers.
#include "main/dllsymbol.h"

// Standard headers.
#include <cstddef>

// Forward declarations.
namespace renderer  { class ParamArray; }

namespace renderer
{

//
// SubdivisionSurface object (source geometry).
//

class APPLESEED_DLLSYMBOL SubdivisionSurfaceObject
  : public Object
{
  public:

    enum SubdivisionScheme
    {
        SubdivisionSchemeBilinear,
        SubdivisionSchemeCatmullClark,
        SubdivisionSchemeLoop
    };

    // Delete this instance.
    virtual void release() APPLESEED_OVERRIDE;

    // Return a string identifying the model of this object.
    virtual const char* get_model() const APPLESEED_OVERRIDE;

    // Return the subdivision scheme.
    SubdivisionScheme get_subdivision_scheme() const;

    // Compute the local space bounding box of the object over the shutter interval.
    virtual GAABB3 compute_local_bbox() const APPLESEED_OVERRIDE;

    // Return the region kit of the object.
    virtual foundation::Lazy<RegionKit>& get_region_kit() APPLESEED_OVERRIDE;

    // Insert and access vertices.
    void reserve_vertices(const size_t count);
    size_t push_vertex(const GVector3& vertex);
    size_t get_vertex_count() const;
    const GVector3& get_vertex(const size_t index) const;

    // Insert and access faces.
    void reserve_faces(const size_t count);
    size_t push_face_edge_count(const size_t num_edges);
    size_t get_face_count() const;
    void clear_faces();

    // Insert and access edges.
    void reserve_edges(const size_t count);
    size_t get_face_num_edges(const size_t index) const;
    size_t push_face_vertex(const size_t vertex_index);
    // todo: add some kind of edge access...

    // todo:
    // add creases and corners...

    // Insert and access texture coordinates.
    void reserve_tex_coords(const size_t count);
    size_t push_tex_coords(const GVector2& tex_coords);
    size_t get_tex_coords_count() const;
    GVector2 get_tex_coords(const size_t index) const;

    // todo:
    // add vertex poses here...

    // Insert and access material slots.
    void reserve_material_slots(const size_t count);
    size_t push_material_slot(const char* name);
    virtual size_t get_material_slot_count() const APPLESEED_OVERRIDE;
    virtual const char* get_material_slot(const size_t index) const APPLESEED_OVERRIDE;

  private:
    friend class SubdivisionSurfaceObjectFactory;

    struct Impl;
    Impl*  impl;

    // Constructor.
    SubdivisionSurfaceObject(
        const char*         name,
        const ParamArray&   params);

    // Destructor.
    ~SubdivisionSurfaceObject();
};


//
// SubdivisionSurface object factory.
//

class APPLESEED_DLLSYMBOL SubdivisionSurfaceObjectFactory
{
  public:
    // Return a string identifying this object model.
    static const char* get_model();

    // Create a new subdivmesh object.
    static foundation::auto_release_ptr<SubdivisionSurfaceObject> create(
        const char*         name,
        const ParamArray&   params);
};

}       // namespace renderer

#endif  // !APPLESEED_RENDERER_MODELING_OBJECT_SUBDIVISIONSURFACEOBJECT_H
