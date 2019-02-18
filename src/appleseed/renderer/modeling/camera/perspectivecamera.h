
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

#pragma once

// appleseed.renderer headers.
#include "renderer/modeling/camera/camera.h"

// appleseed.foundation headers.
#include "foundation/math/vector.h"

// Forward declarations.
namespace renderer      { class ParamArray; }
namespace renderer      { class RasterizationCamera; }

namespace renderer
{

//
// Perspective camera base class.
//

class PerspectiveCamera
  : public Camera
{
  public:
    PerspectiveCamera(
        const char*                     name,
        const ParamArray&               params);

    // This method is called once before rendering.
    // Returns true on success, false otherwise.
    bool on_render_begin(
        const Project&                  project,
        const BaseGroup*                parent,
        OnRenderBeginRecorder&          recorder,
        foundation::IAbortSwitch*       abort_switch = nullptr) override;

    // Similar to project_point(), except that the input point is expressed in camera space.
    bool project_camera_space_point(
        const foundation::Vector3f&     point,
        foundation::Vector2f&           ndc) const override;

    // Project a 3D segment back to the film plane. The input segment is expressed in
    // world space. The returned segment is expressed in normalized device coordinates.
    // Returns true if the projection was possible, false otherwise.
    bool project_segment(
        const float                     time,
        const foundation::Vector3f&     a,
        const foundation::Vector3f&     b,
        foundation::Vector2f&           a_ndc,
        foundation::Vector2f&           b_ndc) const override;

    // Return a camera representation suitable for rasterization.
    RasterizationCamera get_rasterization_camera() const override;

  protected:
    // Parameters.
    foundation::Vector2f    m_film_dimensions;      // film dimensions in camera space, in meters
    float                   m_focal_length;         // focal length in camera space, in meters
    float                   m_near_z;               // Z value of the near plane in camera space, in meters
    foundation::Vector2f    m_shift;                // camera shift in camera space, in meters

    // Precomputed values.
    float                   m_rcp_film_width;       // film width reciprocal in camera space
    float                   m_rcp_film_height;      // film height reciprocal in camera space
    float                   m_pixel_area;           // pixel area in meters, in camera space

    // Utility function to retrieve the focal length (in meters) from the camera parameters.
    float extract_focal_length(const float film_width) const;

    // Focal length <-> horizontal field of view conversion functions.
    // Focal length and film width are expressed in meters; horizontal field of view is expressed in radians.
    static float hfov_to_focal_length(const float film_width, const float hfov);
    static float focal_length_to_hfov(const float film_width, const float focal_length);

    // Project points between NDC and camera spaces.
    foundation::Vector3f ndc_to_camera(const foundation::Vector2f& point) const;
    foundation::Vector2f camera_to_ndc(const foundation::Vector3f& point) const;
};

}   // namespace renderer
