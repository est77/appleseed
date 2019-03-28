
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
// Reference:
//
//   Stratified Sampling of Spherical Triangles.
//   https://www.graphics.cornell.edu/pubs/1995/Arv95c.pdf
//

template <typename T>
class SphericalTriangleSampler
{
  public:
    SphericalTriangleSampler(
        const Vector<T, 3>& a,
        const Vector<T, 3>& b,
        const Vector<T, 3>& c,
        const Vector<T, 3>& o)
    {
        m_A = normalize(a - o);
        m_B = normalize(b - o);
        m_C = normalize(c - o);

        const Vector<T, 3> nAB = normalize(cross(m_A, m_B));
        const Vector<T, 3> nBC = normalize(cross(m_B, m_C));
        const Vector<T, 3> nCA = normalize(cross(m_C, m_A));

        m_alpha = std::acos(dot(-nAB, nCA));
        m_beta = std::acos(dot(-nBC, nAB));
        m_gamma = std::acos(dot(-nCA, nBC));
        m_sr = m_alpha + m_beta + m_gamma - Pi<T>();
    }

    T solid_angle() const
    {
        return m_sr;
    }

    Vector<T, 3> sample(const Vector<T,2> s) const
    {
        T m_area = m_sr * s[0];

        T phi = m_area - m_alpha;
        T sin_phi = std::sin(phi);
        T cos_phi = std::cos(phi);

        T cos_c = dot(m_A, m_B);

        T sin_alpha = sin(m_alpha);
        T cos_alpha = cos(m_alpha);

        T u = cos_phi - cos_alpha;
        T v = sin_phi + sin_alpha * cos_c;

        T cos_b_hat =
            ((v * cos_phi - u * sin_phi) * cos_alpha - v) /
            ((v * sin_phi + u * cos_phi) * sin_alpha);

        const Vector<T, 3> C_hat =
            m_A * cos_b_hat + std::sqrt(T(1.0) - square(cos_b_hat)) * ortho_vector(m_C, m_A);
        const T cos_theta = T(1.0) - s[1] * (T(1.0) - dot(C_hat, m_B));

        const Vector<T, 3> P =
            cos_theta * m_B +
            std::sqrt(std::max(T(1.0) - square(cos_theta), T(0.0))) * ortho_vector(C_hat, m_B);

        return P;
    }

  private:
    Vector<T, 3> m_A;
    Vector<T, 3> m_B;
    Vector<T, 3> m_C;

    T m_alpha;
    T m_beta;
    T m_gamma;
    T m_sr;

    static Vector<T, 3> ortho_vector(const Vector<T, 3> x, const Vector<T, 3> y)
    {
        return normalize(x - dot(x, y) * y);
    }
};

}  // namespace foundation
