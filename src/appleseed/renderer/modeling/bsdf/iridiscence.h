
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017 Esteban Tovagliari, The appleseedhq Organization
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

#ifndef APPLESEED_RENDERER_MODELING_BSDF_IRIDISCENCE_H
#define APPLESEED_RENDERER_MODELING_BSDF_IRIDISCENCE_H

// appleseed.renderer headers.
#include "renderer/global/globaltypes.h"

// appleseed.foundation headers.
#include "foundation/math/vector.h"

// Forward declarations.
namespace foundation    { class DictionaryArray; }
namespace renderer      { class InputArray; }

namespace renderer
{

class IridescenceDielectricFun
{
  public:
    IridescenceDielectricFun(
        const float                 surface_ior,
        const float                 film_ior,
        const float                 film_thickness,
        const float                 outside_ior,
        const Spectrum&             reflectance,
        const float                 reflectance_multiplier);

    void operator()(
        const foundation::Vector3f& o,
        const foundation::Vector3f& h,
        const foundation::Vector3f& n,
        Spectrum&                   value) const;

  private:
    const float     m_surface_ior;
    const float     m_film_ior;
    const float     m_film_thickness;
    const float     m_outside_ior;
    const Spectrum& m_reflectance;
    const float     m_reflectance_multiplier;
};

class IridescenceConductorFun
{
  public:
    IridescenceConductorFun(
        const Spectrum&             nt,
        const Spectrum&             kt,
        const float                 film_ior,
        const float                 film_thickness,
        const float                 outside_ior,
        const float                 reflectance_multiplier);

    void operator()(
        const foundation::Vector3f& o,
        const foundation::Vector3f& h,
        const foundation::Vector3f& n,
        Spectrum&                   value) const;

  private:
    const Spectrum& m_nt;
    const Spectrum& m_kt;
    const float     m_film_ior;
    const float     m_film_thickness;
    const float     m_outside_ior;
    const float     m_reflectance_multiplier;

    static void fresnel_phase_exact(
        const float     cos_theta_i,
        const float     eta,
        const Spectrum& n,
        const Spectrum& k,
        Spectrum&       phi_s,
        Spectrum&       phi_p);
};

void declare_iridiscence_inputs(InputArray& inputs);

void add_iridiscence_metadata(foundation::DictionaryArray& metadata);

void compute_thin_film_thickness_and_ior(
    const float min_thickness,
    const float max_thickness,
    const float thickness,
    const float outside_ior,
    float&      dinc,
    float&      ior);

}       // namespace renderer

#endif  // !APPLESEED_RENDERER_MODELING_BSDF_IRIDISCENCE_H
