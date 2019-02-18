
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

// appleseed.renderer headers.
#include "renderer/utility/transformsequence.h"

// appleseed.foundation headers.
#include "foundation/math/aabb.h"
#include "foundation/math/matrix.h"
#include "foundation/math/scalar.h"
#include "foundation/math/transform.h"
#include "foundation/math/vector.h"
#include "foundation/utility/benchmark.h"

using namespace foundation;
using namespace renderer;

BENCHMARK_SUITE(Renderer_Utility_TransformSequence)
{
    struct Fixture
    {
        const AABB3f        m_bbox;
        TransformSequence   m_sequence;
        AABB3f              m_motion_bbox;

        Fixture()
          : m_bbox(Vector3f(-20.0f, -20.0f, -5.0f), Vector3f(-10.0f, -10.0f, 5.0f))
        {
            const Vector3f axis = normalize(Vector3f(0.1f, 0.2f, 1.0f));
            m_sequence.set_transform(
                0.0f,
                Transformf::from_local_to_parent(
                    Matrix4f::make_rotation(axis, 0.0f) *
                    Matrix4f::make_scaling(Vector3f(0.1f))));
            m_sequence.set_transform(
                1.0f,
                Transformf::from_local_to_parent(
                    Matrix4f::make_rotation(axis, Pi<float>() - Pi<float>() / 8) *
                    Matrix4f::make_scaling(Vector3f(0.2f))));
            m_sequence.prepare();
        }
    };

    BENCHMARK_CASE_F(ToParent, Fixture)
    {
        m_motion_bbox = m_sequence.to_parent(m_bbox);
    }
}
