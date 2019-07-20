
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

// Interface header.
#include "backwardlightsampler.h"

// appleseed.renderer headers.
#include "renderer/global/globallogger.h"
#include "renderer/kernel/lighting/lightsample.h"
#include "renderer/kernel/shading/shadingpoint.h"
#include "renderer/modeling/edf/edf.h"
#include "renderer/modeling/light/light.h"
#include "renderer/modeling/material/material.h"
#include "renderer/modeling/scene/scene.h"

// Standard headers.
#include <cassert>
#include <string>

using namespace foundation;
using namespace std;

namespace renderer
{

//
// BackwardLightSampler class implementation.
//

Dictionary BackwardLightSampler::get_params_metadata()
{
    Dictionary metadata;

    metadata.insert(
        "algorithm",
        Dictionary()
            .insert("type", "enum")
            .insert("values", "cdf")
            .insert("default", "cdf")
            .insert("label", "Light Sampler")
            .insert("help", "Light sampling algoritm")
            .insert(
                "options",
                Dictionary()
                    .insert(
                        "cdf",
                        Dictionary()
                            .insert("label", "CDF")
                            .insert("help", "Cumulative Distribution Function"))));

    metadata.merge(LightSamplerBase::get_params_metadata());

    return metadata;
}

BackwardLightSampler::BackwardLightSampler(
    const Scene&                        scene,
    const ParamArray&                   params)
  : LightSamplerBase(scene, params)
{
}

void BackwardLightSampler::sample_lightset(
    const float                         time,
    const Vector3f&                     s,
    const ShadingPoint&                 shading_point,
    LightSample&                        light_sample) const
{
    // CDF-based sampling.
    sample_emitting_shapes(
        time,
        s,
        light_sample);
}

float BackwardLightSampler::evaluate_pdf(
    const ShadingPoint&                 light_shading_point,
    const ShadingPoint&                 surface_shading_point) const
{
    assert(light_shading_point.is_triangle_primitive());

    const EmittingShapeKey shape_key(
        light_shading_point.get_assembly_instance().get_uid(),
        light_shading_point.get_object_instance_index(),
        light_shading_point.get_primitive_index());

    const auto* shape_ptr = m_emitting_shape_hash_table.get(shape_key);

    if (shape_ptr == nullptr)
        return 0.0f;

    const EmittingShape* shape = *shape_ptr;

    const float shape_probability =  shape->evaluate_pdf_uniform();

    assert(shape_probability >= 0.0f);

    return shape_probability;
}

}   // namespace renderer
