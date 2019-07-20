
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
#include "camera.h"

// appleseed.renderer headers.
#include "renderer/global/globallogger.h"
#include "renderer/kernel/shading/shadingray.h"
#include "renderer/modeling/scene/visibilityflags.h"
#include "renderer/utility/paramarray.h"

// appleseed.foundation headers.
#include "foundation/math/scalar.h"
#include "foundation/utility/api/apistring.h"
#include "foundation/utility/api/specializedapiarrays.h"
#include "foundation/utility/containers/dictionary.h"
#include "foundation/utility/iostreamop.h"

// Standard headers.
#include <cmath>
#include <limits>

using namespace foundation;
using namespace std;

namespace renderer
{

//
// Camera class implementation.
//

namespace
{
    const UniqueID g_class_uid = new_guid();

    const float ShutterOpenBeginTimeDefaultValue = 0.0f;
    const float ShutterCloseEndTimeDefaultValue = 0.0f;
}

UniqueID Camera::get_class_uid()
{
    return g_class_uid;
}

Camera::Camera(
    const char*             name,
    const ParamArray&       params)
  : ConnectableEntity(g_class_uid, params)
{
    set_name(name);
}

bool Camera::on_render_begin(
    const Project&          project,
    const BaseGroup*        parent,
    OnRenderBeginRecorder&  recorder,
    IAbortSwitch*           abort_switch)
{
    if (!Entity::on_render_begin(project, parent, recorder, abort_switch))
        return false;

    m_shutter_open_time =
        m_params.get_optional<float>(
            "shutter_open_time",
            ShutterOpenBeginTimeDefaultValue);

    m_shutter_close_time =
        m_params.get_optional<float>(
            "shutter_close_time",
            ShutterCloseEndTimeDefaultValue);

    if (m_shutter_open_time > m_shutter_close_time)
    {
        RENDERER_LOG_WARNING(
            "while defining camera \"%s\": shutter times are not properly ordered; "
            "order should be: open time <= close time.",
            get_path().c_str());

        m_shutter_close_time = m_shutter_open_time;
    }

    m_shutter_time_interval = m_shutter_close_time - m_shutter_open_time;

    return true;
}

bool Camera::on_frame_begin(
    const Project&          project,
    const BaseGroup*        parent,
    OnFrameBeginRecorder&   recorder,
    IAbortSwitch*           abort_switch)
{
    if (!ConnectableEntity::on_frame_begin(project, parent, recorder, abort_switch))
        return false;

    m_transform_sequence.optimize();

    if (!m_transform_sequence.prepare())
        RENDERER_LOG_WARNING("camera \"%s\" has one or more invalid transforms.", get_path().c_str());

    return true;
}

bool Camera::project_point(
    const float             time,
    const Vector3d&         point,
    Vector2d&               ndc) const
{
    // Retrieve the camera transform.
    Transformd scratch;
    const Transformd& transform = m_transform_sequence.evaluate(time, scratch);

    // Transform the point from world space to camera space.
    const Vector3d point_camera = transform.point_to_local(point);

    return project_camera_space_point(point_camera, ndc);
}

Vector2d Camera::extract_film_dimensions() const
{
    const Vector2d DefaultFilmDimensions(0.025, 0.025);     // in meters
    const double DefaultAspectRatio = DefaultFilmDimensions[0] / DefaultFilmDimensions[1];

    Vector2d film_dimensions;

    if (has_params("film_width", "film_height"))
    {
        film_dimensions[0] = get_greater_than_zero("film_width", DefaultFilmDimensions[0]);
        film_dimensions[1] = get_greater_than_zero("film_height", DefaultFilmDimensions[1]);
    }
    else if (has_params("film_width", "aspect_ratio"))
    {
        const double aspect_ratio = get_greater_than_zero("aspect_ratio", DefaultAspectRatio);
        film_dimensions[0] = get_greater_than_zero("film_width", DefaultFilmDimensions[0]);
        film_dimensions[1] = film_dimensions[0] / aspect_ratio;
    }
    else if (has_params("film_height", "aspect_ratio"))
    {
        const double aspect_ratio = get_greater_than_zero("aspect_ratio", DefaultAspectRatio);
        film_dimensions[1] = get_greater_than_zero("film_height", DefaultFilmDimensions[1]);
        film_dimensions[0] = film_dimensions[1] * aspect_ratio;
    }
    else
    {
        film_dimensions =
            m_params.get_required<Vector2d>("film_dimensions", DefaultFilmDimensions);

        if (film_dimensions[0] <= 0.0 || film_dimensions[1] <= 0.0)
        {
            RENDERER_LOG_ERROR(
                "while defining camera \"%s\": invalid value \"%f %f\" for parameter \"%s\"; "
                "using default value \"%f %f\".",
                get_path().c_str(),
                film_dimensions[0],
                film_dimensions[1],
                "film_dimensions",
                DefaultFilmDimensions[0],
                DefaultFilmDimensions[1]);

            film_dimensions = DefaultFilmDimensions;
        }
    }

    return film_dimensions;
}

double Camera::extract_near_z() const
{
    const double DefaultNearZ = -0.001;         // in meters

    double near_z = m_params.get_optional<double>("near_z", DefaultNearZ);

    if (near_z > 0.0)
    {
        RENDERER_LOG_ERROR(
            "while defining camera \"%s\": invalid near z value \"%f\", near z values must be negative or zero; "
            "using default value \"%f\".",
            get_path().c_str(),
            near_z,
            DefaultNearZ);

        near_z = DefaultNearZ;
    }

    return near_z;
}

Vector2d Camera::extract_shift() const
{
    return Vector2d(
        m_params.get_optional<double>("shift_x", 0.0),
        m_params.get_optional<double>("shift_y", 0.0));
}

void Camera::initialize_ray(
    SamplingContext&        sampling_context,
    ShadingRay&             ray) const
{
    ray.m_tmin = 0.0;
    ray.m_tmax = numeric_limits<double>::max();
    ray.m_flags = VisibilityFlags::CameraRay;
    ray.m_depth = 0;

    sampling_context.split_in_place(1, 1);
    ray.m_time =
        lerp(
            m_shutter_open_time,
            m_shutter_close_time,
            sampling_context.next2<float>());
}

bool Camera::has_param(const char* name) const
{
    return m_params.strings().exist(name);
}

bool Camera::has_params(const char* name1, const char* name2) const
{
    return has_param(name1) && has_param(name2);
}

double Camera::get_greater_than_zero(
    const char*     name,
    const double    default_value) const
{
    const double value = m_params.get_required<double>(name, default_value);

    if (value <= 0.0)
    {
        RENDERER_LOG_ERROR(
            "while defining camera \"%s\": invalid value \"%f\" for parameter \"%s\"; "
            "using default value \"%f\".",
            get_path().c_str(),
            value,
            name,
            default_value);

        return default_value;
    }

    return value;
}


//
// CameraFactory class implementation.
//

DictionaryArray CameraFactory::get_input_metadata()
{
    DictionaryArray metadata;

    metadata.push_back(
        Dictionary()
            .insert("name", "shutter_open_time")
            .insert("label", "Shutter Open Time")
            .insert("type", "numeric")
            .insert("min",
                Dictionary()
                    .insert("value", "0.0")
                    .insert("type", "soft"))
            .insert("max",
                Dictionary()
                    .insert("value", "1.0")
                    .insert("type", "soft"))
            .insert("use", "optional")
            .insert("default", "0.0"));

    metadata.push_back(
        Dictionary()
            .insert("name", "shutter_close_time")
            .insert("label", "Shutter Close Time")
            .insert("type", "numeric")
            .insert("min",
                Dictionary()
                    .insert("value", "0.0")
                    .insert("type", "soft"))
            .insert("max",
                Dictionary()
                    .insert("value", "1.0")
                    .insert("type", "soft"))
            .insert("use", "optional")
            .insert("default", "1.0"));

    return metadata;
}

void CameraFactory::add_film_metadata(DictionaryArray& metadata)
{
    metadata.push_back(
        Dictionary()
            .insert("name", "film_dimensions")
            .insert("label", "Film Dimensions")
            .insert("type", "text")
            .insert("use", "required"));

    metadata.push_back(
        Dictionary()
            .insert("name", "film_width")
            .insert("label", "Film Width")
            .insert("type", "text")
            .insert("use", "required"));

    metadata.push_back(
        Dictionary()
            .insert("name", "film_height")
            .insert("label", "Film Height")
            .insert("type", "text")
            .insert("use", "required"));

    metadata.push_back(
        Dictionary()
            .insert("name", "aspect_ratio")
            .insert("label", "Aspect Ratio")
            .insert("type", "text")
                .insert("use", "required"));
}

void CameraFactory::add_lens_metadata(DictionaryArray& metadata)
{
    metadata.push_back(
        Dictionary()
            .insert("name", "focal_length")
            .insert("label", "Focal Length")
            .insert("type", "text")
            .insert("use", "required"));

    metadata.push_back(
        Dictionary()
            .insert("name", "horizontal_fov")
            .insert("label", "Horizontal FOV")
            .insert("type", "numeric")
            .insert("min",
                Dictionary()
                    .insert("value", "1.0")
                    .insert("type", "soft"))
            .insert("max",
                Dictionary()
                    .insert("value", "180.0")
                    .insert("type", "soft"))
                .insert("use", "required"));
}

void CameraFactory::add_clipping_metadata(DictionaryArray& metadata)
{
    metadata.push_back(
        Dictionary()
            .insert("name", "near_z")
            .insert("label", "Near Z")
            .insert("type", "text")
            .insert("use", "optional")
            .insert("default", "-0.001"));
}

void CameraFactory::add_shift_metadata(DictionaryArray& metadata)
{
    metadata.push_back(
        Dictionary()
            .insert("name", "shift_x")
            .insert("label", "Shift X")
            .insert("type", "text")
            .insert("use", "optional")
            .insert("default", "0.0"));

    metadata.push_back(
        Dictionary()
            .insert("name", "shift_y")
            .insert("label", "Shift Y")
            .insert("type", "text")
            .insert("use", "optional")
            .insert("default", "0.0"));
}

}   // namespace renderer
