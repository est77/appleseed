
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
#include "renderer/global/globaltypes.h"
#include "renderer/modeling/entity/entity.h"
#include "renderer/modeling/scene/basegroup.h"
#include "renderer/modeling/scene/containers.h"
#include "renderer/modeling/scene/iassemblyfactory.h"

// appleseed.foundation headers.
#include "foundation/platform/compiler.h"
//#include "foundation/utility/api/apiarray.h"
#include "foundation/utility/autoreleaseptr.h"
#include "foundation/utility/uid.h"

// appleseed.main headers.
#include "main/dllsymbol.h"

// Standard headers.
#include <cassert>
#include <cstddef>
#include <utility>

// Forward declarations.
namespace foundation    { class Dictionary; }
namespace foundation    { class DictionaryArray; }
namespace foundation    { class IAbortSwitch; }
namespace foundation    { class StringArray; }
namespace foundation    { class StringDictionary; }
namespace renderer      { class OnFrameBeginRecorder; }
namespace renderer      { class OnRenderBeginRecorder; }
namespace renderer      { class ParamArray; }
namespace renderer      { class Project; }

namespace renderer
{

//
// An assembly is either entirely self-contained, or it references colors,
// textures and texture instances defined in the parent scene or assembly.
//

class APPLESEED_DLLSYMBOL Assembly
  : public Entity
  , public BaseGroup
{
  public:
    // Return a string identifying the model of this entity.
    virtual const char* get_model() const;

    // Return the unique ID of this class of entities.
    static foundation::UniqueID get_class_uid();

    // Delete this instance.
    void release() override;

    // Compute the local space bounding box of the assembly, including all child assemblies,
    // over the shutter interval.
    GAABB3 compute_local_bbox() const;

    // Compute the local space bounding box of this assembly, excluding all child assemblies,
    // over the shutter interval.
    GAABB3 compute_non_hierarchical_local_bbox() const;

    bool on_frame_begin(
        const Project&              project,
        const BaseGroup*            parent,
        OnFrameBeginRecorder&       recorder,
        foundation::IAbortSwitch*   abort_switch = nullptr) override;

    // todo (est.): remove me asap.!!!
    void do_bump_version_id() override;

  private:
    friend class AssemblyFactory;

    // Constructor.
    Assembly(
        const char*                 name,
        const ParamArray&           params);
};

//
// This overloads are needed to avoid compiler errors
// because the methods they call are defined in both Entity
// and BaseGroup base classes.
//

void invoke_collect_asset_paths(
    AssemblyContainer&                  assemblies,
    foundation::StringArray&            paths);

void invoke_update_asset_paths(
    AssemblyContainer&                  assemblies,
    const foundation::StringDictionary& mappings);

bool invoke_on_render_begin(
    AssemblyContainer&                  assemblies,
    const Project&                      project,
    const BaseGroup*                    parent,
    OnRenderBeginRecorder&              recorder,
    foundation::IAbortSwitch*           abort_switch);

//
// Assembly factory.
//


class APPLESEED_DLLSYMBOL AssemblyFactory
  : public IAssemblyFactory
{
  public:
    // Delete this instance.
    void release() override;

    // Return a string identifying this assembly model.
    const char* get_model() const override;

    // Return metadata for this assembly model.
    foundation::Dictionary get_model_metadata() const override;

    // Return metadata for the inputs of this assembly model.
    foundation::DictionaryArray get_input_metadata() const override;

    // Create a new assembly.
    foundation::auto_release_ptr<Assembly> create(
        const char*         name,
        const ParamArray&   params = ParamArray()) const override;
};

}   // namespace renderer
