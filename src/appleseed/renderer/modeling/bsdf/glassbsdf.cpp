
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
#include "glassbsdf.h"

// appleseed.renderer headers.
#include "renderer/kernel/lighting/scatteringmode.h"
#include "renderer/modeling/bsdf/bsdf.h"
#include "renderer/modeling/bsdf/bsdfwrapper.h"
#include "renderer/modeling/bsdf/microfacethelper.h"
#include "renderer/modeling/input/inputevaluator.h"
#include "renderer/utility/messagecontext.h"
#include "renderer/utility/paramarray.h"

// appleseed.foundation headers.
#include "foundation/math/sampling/mappings.h"
#include "foundation/math/basis.h"
#include "foundation/math/microfacet.h"
#include "foundation/math/minmax.h"
#include "foundation/math/vector.h"
#include "foundation/utility/containers/dictionary.h"
#include "foundation/utility/containers/specializedarrays.h"
#include "foundation/utility/makevector.h"
#include "foundation/utility/otherwise.h"

// Standard headers.
#include <algorithm>
#include <cmath>
#include <string>

// Forward declarations.
namespace foundation    { class IAbortSwitch; }
namespace renderer      { class Assembly; }
namespace renderer      { class Project; }

using namespace foundation;
using namespace std;

namespace renderer
{

namespace
{
    //
    // Glass BSDF.
    //
    //    A future version of this BSDF will support multiple-scattering.
    //    For that reason, the only available microfacet distribution functions
    //    are those that support it (Beckmann and GGX).
    //
    // References:
    //
    //   [1] Microfacet Models for Refraction through Rough Surfaces.
    //       http://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
    //
    //   [2] Extending the Disney BRDF to a BSDF with Integrated Subsurface Scattering.
    //       http://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_slides.pdf
    //

    const char* Model = "glass_bsdf";

    //
    // The GlassBSDF is used in two different contexts,
    // as an appleseed BSDF and as an OSL closure.
    //
    //  - When used as an appleseed BSDF, the normal is flipped
    //    when shading a backfacing point.
    //
    //  - When used as an OSL closure, the normal is not flipped
    //    when shading a backfacing point.
    //
    // To handle the two cases in an uniform way, the BSDF accepts a
    // backfacing policy class as a template parameter.
    //

    struct AppleseedBackfacingPolicy
    {
        AppleseedBackfacingPolicy(
            const Basis3d&  shading_basis,
            const bool      backfacing)
          : m_basis(
                backfacing ? -shading_basis.get_normal() : shading_basis.get_normal(),
                shading_basis.get_tangent_u(),
                backfacing ? -shading_basis.get_tangent_v() : shading_basis.get_tangent_v())
        {
        }

        const Vector3d& get_normal() const
        {
            return m_basis.get_normal();
        }

        const Vector3d transform_to_local(const Vector3d& v) const
        {
            return m_basis.transform_to_local(v);
        }

        const Vector3d transform_to_parent(const Vector3d& v) const
        {
            return m_basis.transform_to_parent(v);
        }

        const Basis3d     m_basis;
    };

    struct OSLBackfacingPolicy
    {
        OSLBackfacingPolicy(
            const Basis3d&  shading_basis,
            const bool      backfacing)
          : m_basis(shading_basis)
        {
        }

        const Vector3d& get_normal() const
        {
            return m_basis.get_normal();
        }

        const Vector3d transform_to_local(const Vector3d& v) const
        {
            return m_basis.transform_to_local(v);
        }

        const Vector3d transform_to_parent(const Vector3d& v) const
        {
            return m_basis.transform_to_parent(v);
        }

        const Basis3d&  m_basis;
    };

    template <typename BackfacingPolicy>
    class GlassBSDFImpl
      : public BSDF
    {
      public:
        GlassBSDFImpl(
            const char*         name,
            const ParamArray&   params)
          : BSDF(name, AllBSDFTypes, ScatteringMode::Glossy, params)
        {
            m_inputs.declare("surface_transmittance", InputFormatSpectralReflectance);
            m_inputs.declare("surface_transmittance_multiplier", InputFormatScalar, "1.0");
            m_inputs.declare("reflection_tint", InputFormatSpectralReflectance, "1.0");
            m_inputs.declare("refraction_tint", InputFormatSpectralReflectance, "1.0");
            m_inputs.declare("roughness", InputFormatScalar, "0.15");
            m_inputs.declare("anisotropic", InputFormatScalar, "0.0");
            m_inputs.declare("ior", InputFormatScalar, "1.5");
            m_inputs.declare("volume_transmittance", InputFormatSpectralReflectance, "1.0");
            m_inputs.declare("volume_transmittance_distance", InputFormatScalar, "0.0");
        }

        virtual void release() APPLESEED_OVERRIDE
        {
            delete this;
        }

        virtual const char* get_model() const APPLESEED_OVERRIDE
        {
            return Model;
        }

        virtual bool on_frame_begin(
            const Project&      project,
            const Assembly&     assembly,
            IAbortSwitch*       abort_switch) APPLESEED_OVERRIDE
        {
            if (!BSDF::on_frame_begin(project, assembly, abort_switch))
                return false;

            const EntityDefMessageContext context("bsdf", this);
            const string mdf =
                m_params.get_required<string>(
                    "mdf",
                    "ggx",
                    make_vector("beckmann", "ggx"),
                    context);

            if (mdf == "ggx")
                m_mdf.reset(new GGXMDF<double>());
            else if (mdf == "beckmann")
                m_mdf.reset(new BeckmannMDF<double>());
            else return false;

            return true;
        }

        virtual size_t compute_input_data_size(
            const Assembly&     assembly) const APPLESEED_OVERRIDE
        {
            return align(sizeof(InputValues), 16);
        }

        APPLESEED_FORCE_INLINE virtual void prepare_inputs(
            const ShadingPoint& shading_point,
            void*               data) const APPLESEED_OVERRIDE
        {
            InputValues* values = static_cast<InputValues*>(data);

            if (shading_point.is_entering())
            {
                values->m_backfacing = false;
                values->m_eta = shading_point.get_ray().get_current_ior() / values->m_ior;
            }
            else
            {
                values->m_backfacing = true;
                values->m_eta = values->m_ior / shading_point.get_ray().get_previous_ior();
            }

            values->m_reflection_color  = values->m_surface_transmittance;
            values->m_reflection_color *= values->m_reflection_tint;
            values->m_reflection_color *= static_cast<float>(values->m_surface_transmittance_multiplier);

            // [2] Surface absorption, page 5.
            values->m_refraction_color  = values->m_surface_transmittance;
            values->m_refraction_color *= values->m_refraction_tint;
            values->m_refraction_color *= static_cast<float>(values->m_surface_transmittance_multiplier);
            values->m_refraction_color  = sqrt(values->m_refraction_color);

            // Weights used when choosing reflection or refraction.
            values->m_reflection_weight = max_value(values->m_reflection_color);
            values->m_refraction_weight = max_value(values->m_refraction_color);
        }

        APPLESEED_FORCE_INLINE virtual void sample(
            SamplingContext&    sampling_context,
            const void*         data,
            const bool          adjoint,
            const bool          cosine_mult,
            BSDFSample&         sample) const APPLESEED_OVERRIDE
        {
            const InputValues* values = static_cast<const InputValues*>(data);
            const BackfacingPolicy backfacing_policy(sample.get_shading_basis(), values->m_backfacing);

            double alpha_x, alpha_y;
            microfacet_alpha_from_roughness(
                values->m_roughness,
                values->m_anisotropic,
                alpha_x,
                alpha_y);

            const Vector3d wo = backfacing_policy.transform_to_local(
                sample.m_outgoing.get_value());

            // Compute the microfacet normal by sampling the MDF.
            sampling_context.split_in_place(4, 1);
            const Vector4d s = sampling_context.next_vector2<4>();
            Vector3d m = m_mdf->sample(wo, Vector3d(s[0], s[1], s[2]), alpha_x, alpha_y);
            assert(m.y > 0.0);

            const double cos_wom = dot(wo, m);
            double cos_theta_t;
            const double F = fresnel_reflectance(cos_wom, values->m_eta, cos_theta_t);

            const double r_probability = choose_reflection_probability(values, F);

            bool is_refraction;
            Vector3d wi;

            // Choose between reflection and refraction.
            if (s[3] < r_probability)
            {
                // Reflection.
                is_refraction = false;

                // Compute the reflected direction.
                wi = reflect(wo, m);

                // If incoming and outgoing are on different sides
                // of the surface, this is not a reflection.
                if (wi.y * wo.y <= 0.0)
                    return;

                evaluate_reflection(
                    values,
                    wi,
                    wo,
                    m,
                    alpha_x,
                    alpha_y,
                    F,
                    sample.m_value);

                sample.m_probability = reflection_pdf(
                    r_probability,
                    wo,
                    m,
                    cos_wom,
                    alpha_x,
                    alpha_y);
            }
            else
            {
                // Refraction.
                is_refraction = true;

                // Compute the refracted direction.
                wi =
                    cos_wom > 0.0
                        ? (values->m_eta * cos_wom - cos_theta_t) * m - values->m_eta * wo
                        : (values->m_eta * cos_wom + cos_theta_t) * m - values->m_eta * wo;
                wi = improve_normalization(wi);

                // If incoming and outgoing are on the same side
                // of the surface, this is not a refraction.
                if (wi.y * wo.y > 0.0)
                    return;

                evaluate_refraction(
                    values,
                    adjoint,
                    wi,
                    wo,
                    m,
                    alpha_x,
                    alpha_y,
                    1.0 - F,
                    sample.m_value);

                sample.m_probability = refraction_pdf(
                    1.0 - r_probability,
                    wi,
                    wo,
                    m,
                    alpha_x,
                    alpha_y,
                    values->m_eta);
            }

            if (sample.m_probability < 1e-10)
                return;

            sample.m_mode = ScatteringMode::Glossy;
            sample.m_incoming = Dual3d(backfacing_policy.transform_to_parent(wi));

            if (is_refraction)
                sample.compute_transmitted_differentials(values->m_eta);
            else sample.compute_reflected_differentials();
        }

        APPLESEED_FORCE_INLINE virtual double evaluate(
            const void*         data,
            const bool          adjoint,
            const bool          cosine_mult,
            const Vector3d&     geometric_normal,
            const Basis3d&      shading_basis,
            const Vector3d&     outgoing,
            const Vector3d&     incoming,
            const int           modes,
            Spectrum&           value) const APPLESEED_OVERRIDE
        {
            if (!ScatteringMode::has_glossy(modes))
                return 0.0;

            const InputValues* values = static_cast<const InputValues*>(data);
            const BackfacingPolicy backfacing_policy(shading_basis, values->m_backfacing);

            double alpha_x, alpha_y;
            microfacet_alpha_from_roughness(
                values->m_roughness,
                values->m_anisotropic,
                alpha_x,
                alpha_y);

            const Vector3d wi = backfacing_policy.transform_to_local(incoming);
            const Vector3d wo = backfacing_policy.transform_to_local(outgoing);

            Vector3d m;
            double pdf;

            if (wi.y * wo.y > 0.0)
            {
                // Reflection.
                m = half_reflection_vector(wi, wo);
                const double cos_wom = dot(wo, m);
                const double F = fresnel_reflectance(cos_wom, values->m_eta);

                evaluate_reflection(
                    values,
                    wi,
                    wo,
                    m,
                    alpha_x,
                    alpha_y,
                    F,
                    value);

                pdf = reflection_pdf(
                    choose_reflection_probability(values, F),
                    wo,
                    m,
                    cos_wom,
                    alpha_x,
                    alpha_y);
            }
            else
            {
                // Refraction.
                m = half_refraction_vector(wi, wo, values->m_eta);
                const double cos_wom = dot(wo, m);
                const double F = fresnel_reflectance(cos_wom, values->m_eta);

                evaluate_refraction(
                    values,
                    adjoint,
                    wi,
                    wo,
                    m,
                    alpha_x,
                    alpha_y,
                    1.0 - F,
                    value);

                pdf = refraction_pdf(
                    1.0 - choose_reflection_probability(values, F),
                    wi,
                    wo,
                    m,
                    alpha_x,
                    alpha_y,
                    values->m_eta);
            }

            return pdf;
        }

        APPLESEED_FORCE_INLINE virtual double evaluate_pdf(
            const void*         data,
            const Vector3d&     geometric_normal,
            const Basis3d&      shading_basis,
            const Vector3d&     outgoing,
            const Vector3d&     incoming,
            const int           modes) const APPLESEED_OVERRIDE
        {
            if (!ScatteringMode::has_glossy(modes))
                return 0.0;

            const InputValues* values = static_cast<const InputValues*>(data);
            const BackfacingPolicy backfacing_policy(shading_basis, values->m_backfacing);

            double alpha_x, alpha_y;
            microfacet_alpha_from_roughness(
                values->m_roughness,
                values->m_anisotropic,
                alpha_x,
                alpha_y);

            const Vector3d wi = backfacing_policy.transform_to_local(incoming);
            const Vector3d wo = backfacing_policy.transform_to_local(outgoing);

            Vector3d m;

            if (wi.y * wo.y > 0.0)
            {
                // Reflection.
                m = half_reflection_vector(wi, wo);
                const double cos_wom = dot(wo, m);
                const double F = fresnel_reflectance(cos_wom, values->m_eta);

                return reflection_pdf(
                    choose_reflection_probability(values, F),
                    wo,
                    m,
                    cos_wom,
                    alpha_x,
                    alpha_y);
            }
            else
            {
                // Refraction.
                m = half_refraction_vector(wi, wo, values->m_eta);
                const double cos_wom = dot(wo, m);
                const double F = fresnel_reflectance(cos_wom, values->m_eta);

                return refraction_pdf(
                    1.0 - choose_reflection_probability(values, F),
                    wi,
                    wo,
                    m,
                    alpha_x,
                    alpha_y,
                    values->m_eta);
            }
        }

        virtual double sample_ior(
            SamplingContext&    sampling_context,
            const void*         data) const APPLESEED_OVERRIDE
        {
            return static_cast<const InputValues*>(data)->m_ior;
        }

        virtual void compute_absorption(
            const void*         data,
            const double        distance,
            Spectrum&           absorption) const APPLESEED_OVERRIDE
        {
            const InputValues* values = static_cast<const InputValues*>(data);

            if (values->m_volume_transmittance_distance != 0.0)
            {
                // [2] Volumetric absorption reparameterization, page 5.
                absorption.resize(values->m_volume_transmittance.size());
                const float d = static_cast<float>(distance / values->m_volume_transmittance_distance);
                for (size_t i = 0, e = absorption.size(); i < e; ++i)
                {
                    const float a = log(max(values->m_volume_transmittance[i], 0.01f));
                    absorption[i] = exp(a * d);
                }
            }
            else
                absorption.set(1.0f);
        }

      private:
        typedef GlassBSDFInputValues InputValues;

        auto_ptr<MDF<double> > m_mdf;

        static double choose_reflection_probability(
            const InputValues   *values,
            const double        F)
        {
            const double r_probability =
                F * values->m_reflection_weight;
            const double t_probability =
                (1.0 - F) * values->m_refraction_weight;

            const double sum_probabilities = r_probability + t_probability;
            if (sum_probabilities <= 0.0)
                return 1.0;

            return r_probability / sum_probabilities;
        }

        static double fresnel_reflectance(
            const double        cos_theta_i,
            const double        eta,
            double&             cos_theta_t)
        {
            const double sin_theta_t2 = (1.0 - square(cos_theta_i)) * square(eta);

            if (sin_theta_t2 > 1.0)
            {
                cos_theta_t = 0.0;
                return 1.0;
            }

            cos_theta_t = sqrt(1.0 - sin_theta_t2);

            double F;
            fresnel_reflectance_dielectric(
                F,
                eta,
                abs(cos_theta_i),
                cos_theta_t);
            return F;
        }

        static double fresnel_reflectance(
            const double        cos_theta_i,
            const double        eta)
        {
            double cos_theta_t;
            return fresnel_reflectance(cos_theta_i, eta, cos_theta_t);
        }

        static Vector3d half_reflection_vector(
            const Vector3d&     wi,
            const Vector3d&     wo)
        {
            // [1] eq. 13
            const Vector3d h = normalize(wi + wo);
            return h.y < 0.0 ? -h : h;
        }

        void evaluate_reflection(
            const InputValues*  values,
            const Vector3d&     wi,
            const Vector3d&     wo,
            const Vector3d&     m,
            const double        alpha_x,
            const double        alpha_y,
            const double        F,
            Spectrum&           value) const
        {
            // [1] eq. 20.
            const double denom = abs(4.0 * wo.y * wi.y);
            if (denom == 0.0)
            {
                value.set(0.0f);
                return;
            }

            value = values->m_reflection_color;
            const double D = m_mdf->D(m, alpha_x, alpha_y);
            const double G = m_mdf->G(wi, wo, m, alpha_x, alpha_y);
            value *= static_cast<float>(F * D * G / denom);
        }

        double reflection_pdf(
            const double        choose_reflection_probability,
            const Vector3d&     wo,
            const Vector3d&     m,
            const double        cos_wom,
            const double        alpha_x,
            const double        alpha_y) const
        {
            // [1] eq. 14.
            if (cos_wom == 0.0)
                return 0.0;

            const double jacobian = 1.0 / (4.0 * abs(cos_wom));
            return choose_reflection_probability * jacobian * m_mdf->pdf(wo, m, alpha_x, alpha_y);
        }

        static Vector3d half_refraction_vector(
            const Vector3d&     wi,
            const Vector3d&     wo,
            const double        eta)
        {
            // [1] eq. 13
            const Vector3d h = normalize(wi + eta * wo);
            return h.y < 0.0 ? -h : h;
        }

        void evaluate_refraction(
            const InputValues*  values,
            const bool          adjoint,
            const Vector3d&     wi,
            const Vector3d&     wo,
            const Vector3d&     m,
            const double        alpha_x,
            const double        alpha_y,
            const double        T,
            Spectrum&           value) const
        {
            // [1] eq. 21
            const double cos_ih = dot(m, wi);
            const double cos_oh = dot(m, wo);
            const double dots = (cos_ih * cos_oh) / (wi.y * wo.y);

            const double sqrt_denom = cos_ih + values->m_eta * cos_oh;
            if (sqrt_denom == 0.0)
            {
                value.set(0.0f);
                return;
            }

            value = values->m_refraction_color;
            const double D = m_mdf->D(m, alpha_x, alpha_y);
            const double G = m_mdf->G(wi, wo, m, alpha_x, alpha_y);
            value *= static_cast<float>(
                abs(dots) * square(values->m_eta / sqrt_denom) * T * D * G);

            if (adjoint)
                value *= static_cast<float>(square(values->m_eta));
        }

        double refraction_pdf(
            const double        choose_refraction_probability,
            const Vector3d&     wi,
            const Vector3d&     wo,
            const Vector3d&     m,
            const double        alpha_x,
            const double        alpha_y,
            const double        eta) const
        {
            // [1] eq. 17
            const double cos_ih = dot(m, wi);
            const double cos_oh = dot(m, wo);

            const double sqrt_denom = cos_ih + eta * cos_oh;
            if (sqrt_denom == 0.0)
                return 0.0;

            const double jacobian = abs(cos_oh) * square(eta / sqrt_denom);
            return choose_refraction_probability * jacobian * m_mdf->pdf(wo, m, alpha_x, alpha_y);
        }
    };

    typedef
        BSDFWrapper<
            GlassBSDFImpl<AppleseedBackfacingPolicy> > AppleseedGlassBSDF;

    typedef
        BSDFWrapper<
            GlassBSDFImpl<OSLBackfacingPolicy> > OSLGlassBSDF;
}


//
// GlassBSDFFactory class implementation.
//

const char* GlassBSDFFactory::get_model() const
{
    return Model;
}

Dictionary GlassBSDFFactory::get_model_metadata() const
{
    return
        Dictionary()
            .insert("name", Model)
            .insert("label", "Glass BSDF");
}

DictionaryArray GlassBSDFFactory::get_input_metadata() const
{
    DictionaryArray metadata;

    metadata.push_back(
        Dictionary()
            .insert("name", "mdf")
            .insert("label", "Microfacet Distribution Function")
            .insert("type", "enumeration")
            .insert("items",
                Dictionary()
                    .insert("Beckmann", "beckmann")
                    .insert("GGX", "ggx"))
            .insert("use", "required")
            .insert("default", "ggx"));

    metadata.push_back(
        Dictionary()
            .insert("name", "surface_transmittance")
            .insert("label", "Surface Transmittance")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary()
                    .insert("color", "Colors")
                    .insert("texture_instance", "Textures"))
            .insert("use", "required")
            .insert("default", "0.85"));

    metadata.push_back(
        Dictionary()
            .insert("name", "surface_transmittance_multiplier")
            .insert("label", "Surface Transmittance Multiplier")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary().insert("texture_instance", "Textures"))
            .insert("use", "optional")
            .insert("default", "1.0"));

    metadata.push_back(
        Dictionary()
            .insert("name", "reflection_tint")
            .insert("label", "Reflection Tint")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary()
                    .insert("color", "Colors")
                    .insert("texture_instance", "Textures"))
            .insert("use", "optional")
            .insert("default", "1.0"));

    metadata.push_back(
        Dictionary()
            .insert("name", "refraction_tint")
            .insert("label", "Refraction Tint")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary()
                    .insert("color", "Colors")
                    .insert("texture_instance", "Textures"))
            .insert("use", "optional")
            .insert("default", "1.0"));

    metadata.push_back(
        Dictionary()
            .insert("name", "ior")
            .insert("label", "Index of Refraction")
            .insert("type", "numeric")
            .insert("min_value", "1.0")
            .insert("max_value", "2.5")
            .insert("use", "required")
            .insert("default", "1.5"));

    metadata.push_back(
        Dictionary()
            .insert("name", "roughness")
            .insert("label", "Roughness")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary()
                    .insert("color", "Colors")
                    .insert("texture_instance", "Textures"))
            .insert("use", "optional")
            .insert("min_value", "0.0")
            .insert("max_value", "1.0")
            .insert("default", "0.15"));

    metadata.push_back(
        Dictionary()
            .insert("name", "anisotropic")
            .insert("label", "Anisotropic")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary()
                    .insert("color", "Colors")
                    .insert("texture_instance", "Textures"))
            .insert("use", "optional")
            .insert("min_value", "-1.0")
            .insert("max_value", "1.0")
            .insert("default", "0.0"));

    metadata.push_back(
        Dictionary()
            .insert("name", "volume_transmittance")
            .insert("label", "Volume Transmittace")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary()
                    .insert("color", "Colors")
                    .insert("texture_instance", "Textures"))
            .insert("use", "optional")
            .insert("default", "1.0"));

    metadata.push_back(
        Dictionary()
            .insert("name", "volume_transmittance_distance")
            .insert("label", "Volume Transmittance Distance")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary()
                    .insert("color", "Colors")
                    .insert("texture_instance", "Textures"))
            .insert("use", "optional")
            .insert("min_value", "0.0")
            .insert("default", "0.0"));

    return metadata;
}

auto_release_ptr<BSDF> GlassBSDFFactory::create(
    const char*         name,
    const ParamArray&   params) const
{
    return auto_release_ptr<BSDF>(new AppleseedGlassBSDF(name, params));
}

auto_release_ptr<BSDF> GlassBSDFFactory::create_osl(
    const char*         name,
    const ParamArray&   params) const
{
    return auto_release_ptr<BSDF>(new OSLGlassBSDF(name, params));
}

auto_release_ptr<BSDF> GlassBSDFFactory::static_create(
    const char*         name,
    const ParamArray&   params)
{
    return auto_release_ptr<BSDF>(new AppleseedGlassBSDF(name, params));
}

}   // namespace renderer
