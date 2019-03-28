
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2019 Esteban Tovagliari, The appleseedhq Organization
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

// appleseed.foundation headers.
#include "foundation/math/scalar.h"
#include "foundation/math/vector.h"

// Standard headers.
#include <cassert>
#include <cmath>

namespace foundation
{

//
// References:
//
//   An Area-Preserving Parametrization for Spherical Rectangles.
//   https://www.arnoldrenderer.com/research/egsr2013_spherical_rectangle.pdf
//

template <typename T>
class SphericalRectangleSampler
{
  public:
    SphericalRectangleSampler(
        const Vector<T, 3>& origin,
        const Vector<T, 3>& x,
        const Vector<T, 3>& y,
        const Vector<T, 3>& n,
        const Vector<T, 3>& o)
      : m_x(x)
      , m_y(y)
      , m_z(n)
      , m_o(o)
    {
        const Vector<T, 3> d = origin - o;

        T ex_len = norm(x);
        const T x0 = dot(d, x);
        const T x1 = x0 + ex_len;

        T ey_len = norm(y);
        m_y0 = dot(d, y);
        m_y1 = m_y0 + ey_len;

        m_z0 = dot(d, n);

        // z flip
        if (m_z0 > T(0.0))
        {
            m_z0 *= T(-1.0);
            m_z  *= T(-1.0);
        }

        m_n[0] = Vector<T, 3>(T(0.0), m_z0, -m_y0);
        m_n[1] = Vector<T, 3>(-m_z0, T(0.0), x1);
        m_n[2] = Vector<T, 3>(T(0.0), -m_z0, m_y1);
        m_n[3] = Vector<T, 3>(m_z0, T(0.0), -x0);


        m_y0y0 = square(m_y0);
        m_y1y1 = square(m_y1);
        m_z0z0 = square(m_z0);

        m_n[0].z /= std::sqrt(m_z0z0 + m_y0y0);
        m_n[1].z /= std::sqrt(m_z0z0 + square(x1));
        m_n[2].z /= std::sqrt(m_z0z0 + m_y1y1);
        m_n[3].z /= std::sqrt(m_z0z0 + square(x0));

        m_g[0] = std::acos(-m_n[0].z * m_n[1].z);
        m_g[1] = std::acos(-m_n[1].z * m_n[2].z);
        m_g[2] = std::acos(-m_n[2].z * m_n[3].z);
        m_g[3] = std::acos(-m_n[3].z * m_n[0].z);

        m_sr = m_g[0] + m_g[1] + m_g[2] + m_g[3] - TwoPi<T>();
    }

    T solid_angle() const
    {
        return m_sr;
    }

    Vector<T, 3> sample(const Vector<T, 2>& s) const
    {
        const T phi_u = s[0] * m_sr - m_g[2] - m_g[3] + TwoPi<T>();

        const T b0 = m_n[0].z;
        const T b1 = m_n[2].z;
        const T fu = (std::cos(phi_u) * b0 - b1) / std::sin(phi_u);
        const T cu = std::copysign(T(1.0), fu) / std::sqrt(square(fu) + square(b0));
        const T xu = -cu * m_z0 / safe_sqrt(T(1.0) - square(cu));

        const T d = std::sqrt(square(xu) + m_z0z0);
        const T d2 = square(d);
        const T h0 = m_y0 / std::sqrt(d2 + m_y0y0);
        const T h1 = m_y1 / std::sqrt(d2 + m_y1y1);
        const T hv = lerp(h0, h1, s[1]);
        const T yv = hv * d / safe_sqrt(T(1.0) - square(hv));
        return m_o + xu * m_x + yv * m_y + m_z0 * m_z;
    }

  private:
    Vector<T, 3> m_x;
    Vector<T, 3> m_y;
    Vector<T, 3> m_z;
    Vector<T, 3> m_o;

    T            m_y0;
    T            m_y1;
    T            m_z0;
    T            m_z0z0;
    T            m_y0y0;
    T            m_y1y1;

    T            m_sr;

    Vector<T, 3> m_n[4];
    T            m_g[4];

    static T safe_sqrt(const T x)
    {
        return std::sqrt(std::max(x, T(0.0)));
    }
};

}  // namespace foundation
