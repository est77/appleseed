
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

BackwardLightSampler::BackwardLightSampler(
    const Scene&                        scene,
    const ParamArray&                   params)
  : LightSamplerBase(params)
{
    RENDERER_LOG_INFO("collecting light emitters...");

    // Collect all non-physical lights.
    collect_non_physical_lights(
        scene.assembly_instances(),
        TransformSequence());
    m_non_physical_light_count = m_non_physical_lights.size();

    // Collect all light-emitting shapes.
    collect_emitting_shapes(
        scene.assembly_instances(),
        TransformSequence());

    // Build the hash table of emitting shapes.
    build_emitting_shape_hash_table();

    // Prepare the non-physical lights CDF for sampling.
    if (m_non_physical_lights_cdf.valid())
        m_non_physical_lights_cdf.prepare();

    // Prepare the light-emitting shapes CDF for smapling.
    if (m_emitting_shapes_cdf.valid())
        m_emitting_shapes_cdf.prepare();

    // Store the shape probability densities into the emitting shapes.
    for (size_t i = 0, e = m_emitting_shapes.size(); i < e; ++i)
        m_emitting_shapes[i].set_shape_prob(m_emitting_shapes_cdf[i].second);

    RENDERER_LOG_INFO(
        "found %s %s, %s emitting %s.",
        pretty_int(m_non_physical_light_count).c_str(),
        plural(m_non_physical_light_count, "non-physical light").c_str(),
        pretty_int(m_emitting_shapes.size()).c_str(),
        plural(m_emitting_shapes.size(), "shape").c_str());
}

bool BackwardLightSampler::sample_lightset(
    const ShadingRay::Time&             time,
    const Vector3f&                     s,
    const ShadingPoint&                 shading_point,
    LightSample&                        light_sample) const
{
    // CDF-based sampling.
    return sample_emitting_shapes_solid_angle(shading_point, time, s, light_sample);
}

float BackwardLightSampler::evaluate_pdf(
    const ShadingPoint&                 light_shading_point,
    const ShadingPoint&                 surface_shading_point) const
{
    const EmittingShapeKey shape_key(
        light_shading_point.get_assembly_instance().get_uid(),
        light_shading_point.get_object_instance_index(),
        light_shading_point.get_primitive_index());

    const auto* shape_ptr = m_emitting_shape_hash_table.get(shape_key);

    if (shape_ptr == nullptr)
        return 0.0f;

    const EmittingShape* shape = *shape_ptr;
    return shape->evaluate_pdf_solid_angle(
        surface_shading_point.get_point(),
        light_shading_point.get_point());
}

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

    return metadata;
}

}   // namespace renderer
