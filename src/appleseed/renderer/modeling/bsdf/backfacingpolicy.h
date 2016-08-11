
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

#ifndef APPLESEED_RENDERER_MODELING_BSDF_BACKFACING_POLICY_H
#define APPLESEED_RENDERER_MODELING_BSDF_BACKFACING_POLICY_H

// appleseed.foundation headers.
#include "foundation/math/basis.h"
#include "foundation/math/vector.h"

namespace renderer
{

//
//  Many appleseed BSDFs are used in two different contexts,
//  as an appleseed BSDF and as an OSL closure.
//
//  - When used as appleseed BSDFs, the normal is flipped
//    when shading a backfacing point.
//
//  - When used as OSL closures, the normal is not flipped
//    when shading a backfacing point.
//
//  To handle the two cases in an uniform way, some BSDFs accept a
//  backfacing policy class as a template parameter.
//

struct AppleseedBackfacingPolicy
{
    AppleseedBackfacingPolicy(
        const foundation::Basis3d&  shading_basis,
        const bool                  backfacing)
      : m_basis(
            backfacing ? -shading_basis.get_normal() : shading_basis.get_normal(),
            shading_basis.get_tangent_u(),
            backfacing ? -shading_basis.get_tangent_v() : shading_basis.get_tangent_v())
    {
    }

    const foundation::Vector3d& get_normal() const
    {
        return m_basis.get_normal();
    }

    const foundation::Vector3d transform_to_local(const foundation::Vector3d& v) const
    {
        return m_basis.transform_to_local(v);
    }

    const foundation::Vector3d transform_to_parent(const foundation::Vector3d& v) const
    {
        return m_basis.transform_to_parent(v);
    }

    const foundation::Basis3d     m_basis;
};

struct OSLBackfacingPolicy
{
    OSLBackfacingPolicy(
        const foundation::Basis3d&  shading_basis,
        const bool                  backfacing)
      : m_basis(shading_basis)
    {
    }

    const foundation::Vector3d& get_normal() const
    {
        return m_basis.get_normal();
    }

    const foundation::Vector3d transform_to_local(const foundation::Vector3d& v) const
    {
        return m_basis.transform_to_local(v);
    }

    const foundation::Vector3d transform_to_parent(const foundation::Vector3d& v) const
    {
        return m_basis.transform_to_parent(v);
    }

    const foundation::Basis3d&  m_basis;
};

}   // namespace renderer

#endif  // !APPLESEED_RENDERER_MODELING_BSDF_BACKFACING_POLICY_H
