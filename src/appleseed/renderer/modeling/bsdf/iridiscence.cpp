
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

// Interface header.
#include "iridiscence.h"

// appleseed.renderer headers.
#include "renderer/modeling/color/wavelengths.h"
#include "renderer/modeling/input/inputarray.h"

// appleseed.foundation headers.
#include "foundation/math/fresnel.h"
#include "foundation/math/scalar.h"
#include "foundation/utility/api/specializedapiarrays.h"
#include "foundation/utility/containers/dictionary.h"

// Standard headers.
#include <algorithm>

using namespace foundation;
using namespace std;

namespace renderer
{

IridescenceDielectricFun::IridescenceDielectricFun(
    const float     surface_ior,
    const float     film_ior,
    const float     film_thickness,
    const float     outside_ior,
    const Spectrum& reflectance,
    const float     reflectance_multiplier)
  : m_surface_ior(surface_ior)
  , m_film_ior(film_ior)
  , m_film_thickness(film_thickness)
  , m_outside_ior(outside_ior)
  , m_reflectance(reflectance)
  , m_reflectance_multiplier(reflectance_multiplier)
{
}

void IridescenceDielectricFun::operator()(
    const Vector3f& o,
    const Vector3f& h,
    const Vector3f& n,
    Spectrum&       value) const
{
    const float cos_theta_i = dot(o, h);
    const float sin_theta_i2 = 1.0f - square(cos_theta_i);
    const float eta = m_outside_ior / m_film_ior;
    const float sin_theta_t2 = sin_theta_i2 * square(eta);
    const float cos_theta_t2 = 1.0f - sin_theta_t2;

    // Check for total internal reflection.
    if (cos_theta_t2 < 0.0f)
    {
        value.set(1.0f);
        return;
    }

    const float cos_theta_t = sqrt(cos_theta_t2);
    float R12_s, R12_p;
    fresnel_reflectance_dielectric_p(R12_p, m_outside_ior / m_film_ior, cos_theta_i, cos_theta_t);
    fresnel_reflectance_dielectric_s(R12_s, m_outside_ior / m_film_ior, cos_theta_i, cos_theta_t);

    const float T121_p = 1.0f - R12_p;
    const float T121_s = 1.0f - R12_s;

    float F;
    fresnel_reflectance_dielectric_p(F, m_film_ior / m_surface_ior, cos_theta_i, cos_theta_t);
    Spectrum R23_p = m_reflectance; R23_p *= F;

    fresnel_reflectance_dielectric_s(F, m_film_ior / m_surface_ior, cos_theta_i, cos_theta_t);
    Spectrum R23_s = m_reflectance; R23_s *= F;

    // Evaluate the phase shift.
    const float phi21_s = Pi<float>();
    const float phi21_p = Pi<float>();
    const float phi23_s = 0.0f, phi23_p = 0.0f;

    const float d = 2.0f * m_film_ior * m_film_thickness * cos_theta_t;

    value.resize(g_light_wavelengths_nm.size());
    for(size_t i = 0, e = value.size(); i < e; ++i)
    {
        const float d_phi = TwoPi<float>() * d / g_light_wavelengths_nm[i];

        const float R123_p = sqrt(R12_p * R23_p[i]);
        const float R123_s = sqrt(R12_s * R23_s[i]);

        // Iridescence term using Airy summation (Eq. 11) for Parallel polarization
        const float R_p = (square(T121_p) * R23_p[i]) / (1.0f - R12_p * R23_p[i]);
        const float cos_p = cos(d_phi + phi23_p + phi21_p);
        const float irid_p = (R123_p * cos_p - square(R123_p)) / (1.0f - 2.0f * R123_p * cos_p + square(R123_p));
        value[i] = R12_p + R_p + 2.0f * (R_p - T121_p) * irid_p;

        // Iridescence term using Airy summation (Eq. 11) for Perpendicular polarization
        const float R_s = (square(T121_s) * R23_s[i]) / (1.0f - R12_s * R23_s[i]);
        const float cos_s = cos(d_phi + phi23_s + phi21_s);
        const float irid_s = (R123_s * cos_s - square(R123_s)) / (1.0f - 2.0 * R123_s * cos_s + square(R123_s));
        value[i] += R12_s + R_s + 2.0f * (R_s - T121_s) * irid_s;

        value[i] = max(value[i] * 0.5f, 0.0f);
    }

    value *= m_reflectance_multiplier;
}

IridescenceConductorFun::IridescenceConductorFun(
    const Spectrum& nt,
    const Spectrum& kt,
    const float     film_ior,
    const float     film_thickness,
    const float     outside_ior,
    const float     reflectance_multiplier)
  : m_nt(nt)
  , m_kt(kt)
  , m_film_ior(film_ior)
  , m_film_thickness(film_thickness)
  , m_outside_ior(outside_ior)
  , m_reflectance_multiplier(reflectance_multiplier)
{
    assert(m_nt.is_spectral());
    assert(m_kt.is_spectral());
}

void IridescenceConductorFun::operator()(
    const Vector3f& o,
    const Vector3f& h,
    const Vector3f& n,
    Spectrum&       value) const
{
    const float cos_theta_i = dot(o, h);
    const float sin_theta_i2 = 1.0f - square(cos_theta_i);
    const float eta = m_outside_ior / m_film_ior;
    const float sin_theta_t2 = sin_theta_i2 * square(eta);
    const float cos_theta_t2 = 1.0f - sin_theta_t2;

    float R12_s, R12_p, T121_p, T121_s;
    Spectrum R23_p(0.0f), R23_s(0.0f);

    // Check for total internal reflection.
    if (cos_theta_t2 < 0.0f)
    {
        value.set(1.0f);
        return;
    }

    const float cos_theta_t = sqrt(cos_theta_t2);
    fresnel_reflectance_dielectric_p(R12_p, m_outside_ior / m_film_ior, cos_theta_i, cos_theta_t);
    fresnel_reflectance_dielectric_s(R12_s, m_outside_ior / m_film_ior, cos_theta_i, cos_theta_t);

    T121_p = 1.0f - R12_p;
    T121_s = 1.0f - R12_s;

    fresnel_reflectance_conductor_components(
        R23_s,
        R23_p,
        m_nt,
        m_kt,
        m_film_ior,
        cos_theta_t);

    // Evaluate the phase shift.
    float phi21_s = 0.0f, phi21_p = 0.0f;
    //fresnel_phase_exact(cos_theta_i, m_outside_ior, m_film_ior, phi21_s, phi21_p);
    phi21_s = Pi<float>() - phi21_s;
    phi21_p = Pi<float>() - phi21_p;

    Spectrum phi23_s, phi23_p;
    fresnel_phase_exact(cos_theta_t, m_film_ior, m_nt, m_kt, phi23_s, phi23_p);

    const float d = 2.0f * m_film_ior * m_film_thickness * cos_theta_t;

    value.resize(g_light_wavelengths_nm.size());
    for(size_t i = 0, e = value.size(); i < e; ++i)
    {
        const float d_phi = TwoPi<float>() * d / g_light_wavelengths_nm[i];

        const float R123_p = sqrt(R12_p * R23_p[i]);
        const float R123_s = sqrt(R12_s * R23_s[i]);

        // Iridescence term using Airy summation (Eq. 11) for Parallel polarization
        const float R_p = (square(T121_p) * R23_p[i]) / (1.0f - R12_p * R23_p[i]);
        const float cos_p = cos(d_phi + phi23_p[i] + phi21_p);
        const float irid_p = (R123_p * cos_p - square(R123_p)) / (1.0f - 2.0f * R123_p * cos_p + square(R123_p));
        value[i] = R12_p + R_p + 2.0f * (R_p - T121_p) * irid_p;

        // Iridescence term using Airy summation (Eq. 11) for Perpendicular polarization
        const float R_s = (square(T121_s) * R23_s[i]) / (1.0f - R12_s * R23_s[i]);
        const float cos_s = cos(d_phi + phi23_s[i] + phi21_s);
        const float irid_s = (R123_s * cos_s - square(R123_s)) / (1.0f - 2.0 * R123_s * cos_s + square(R123_s));
        value[i] += R12_s + R_s + 2.0f * (R_s - T121_s) * irid_s;

        value[i] = max(value[i] * 0.5f, 0.0f);
    }

    value *= m_reflectance_multiplier;
}

void IridescenceConductorFun::fresnel_phase_exact(
    const float     cos_theta_i,
    const float     eta,
    const Spectrum& n,
    const Spectrum& k,
    Spectrum&       phi_s,
    Spectrum&       phi_p)
{
    assert(n.is_spectral());
    assert(k.is_spectral());

    const float cos_theta_i2 = square(cos_theta_i);
    const float sin_theta2 = 1.0f - cos_theta_i2;

    const float eta2 = square(eta);

    phi_p.resize(n.size());
    phi_s.resize(n.size());

    for (size_t i = 0, e = n.size(); i < e; ++i)
    {
        const float n2 = square(n[i]);
        const float k2 = square(k[i]);

        const float A = n2 * (1.0f - k2) - eta2 * sin_theta2;
        const float B = sqrt(square(A) + square(2.0f * n2 * k[i]));

        const float U = sqrt((A + B) / 2.0f);
        const float U2 = square(U);

        const float V = sqrt((B - A) / 2.0f);
        const float V2 = square(V);

        const float Ys = 2.0f * eta * V * cos_theta_i;
        const float Xs = U2 + V2 - square(eta * cos_theta_i);

        const float Yp = 2.0f * eta * n2 * cos_theta_i * (2.0f * k[i] * U - (1 - k2) * V);
        const float Xp = square(n2 * (1.0f + k2) * cos_theta_i) - eta2 * (U2 + V2);

        phi_s[i] = atan2(Ys, Xs);
        phi_p[i] = atan2(Yp, Xp);
    }
}

void declare_iridiscence_inputs(InputArray& inputs)
{
    inputs.declare("thin_film_ior", InputFormatFloat, "1.3");
    inputs.declare("thin_film_thickness", InputFormatFloat, "0.0");
    inputs.declare("thin_film_min_thickness", InputFormatFloat, "0.0");
    inputs.declare("thin_film_max_thickness", InputFormatFloat, "2000.0");
}

void add_iridiscence_metadata(DictionaryArray& metadata)
{
    metadata.push_back(
        Dictionary()
            .insert("name", "thin_film_ior")
            .insert("label", "Thin Film Index of Refraction")
            .insert("type", "numeric")
            .insert("use", "optional")
            .insert("min",
                Dictionary()
                    .insert("value", "1.0")
                    .insert("type", "hard"))
            .insert("max",
                Dictionary()
                    .insert("value", "2.5")
                    .insert("type", "hard"))
            .insert("default", "1.3"));

    metadata.push_back(
        Dictionary()
            .insert("name", "thin_film_thickness")
            .insert("label", "Thin Film Thickness")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary()
                    .insert("color", "Colors")
                    .insert("texture_instance", "Textures"))
            .insert("use", "optional")
            .insert("min",
                Dictionary()
                    .insert("value", "0.0")
                    .insert("type", "hard"))
            .insert("max",
                Dictionary()
                    .insert("value", "1.0")
                    .insert("type", "hard"))
            .insert("default", "0.0"));

    metadata.push_back(
        Dictionary()
            .insert("name", "thin_film_min_thickness")
            .insert("label", "Thin Film Min Thickness")
            .insert("type", "numeric")
            .insert("use", "optional")
            .insert("min",
                Dictionary()
                    .insert("value", "0.0")
                    .insert("type", "hard"))
            .insert("max",
                Dictionary()
                    .insert("value", "2000.0")
                    .insert("type", "hard"))
            .insert("default", "0.0"));

    metadata.push_back(
        Dictionary()
            .insert("name", "thin_film_max_thickness")
            .insert("label", "Thin Film Max Thickness")
            .insert("type", "numeric")
            .insert("use", "optional")
            .insert("min",
                Dictionary()
                    .insert("value", "0.0")
                    .insert("type", "hard"))
            .insert("max",
                Dictionary()
                    .insert("value", "2000.0")
                    .insert("type", "hard"))
            .insert("default", "2000.0"));
}

void compute_thin_film_thickness_and_ior(
    const float min_thickness,
    const float max_thickness,
    const float thickness,
    const float outside_ior,
    float&      dinc,
    float&      ior)
{
    dinc = lerp(min_thickness, max_thickness, thickness);
    // Force thin_film_ior to outside ior when thickness tends to 0.
    ior = mix(outside_ior, ior, smoothstep(0.0f, 3.0f, dinc));
}

}       // namespace renderer
