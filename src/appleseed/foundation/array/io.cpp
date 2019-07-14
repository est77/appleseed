
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

// Interface header.
#include "io.h"

// appleseed.foundation headers.
#include "foundation/utility/z85.h"

namespace foundation
{

std::string z85_encode(const Array& array)
{
    const size_t array_size_in_bytes = array.size() * array.item_size();
    assert(array_size_in_bytes % 4 == 0);

    const size_t encoded_size = z85_encoded_size(array_size_in_bytes);

    std::string encoded;
    encoded.resize(encoded_size);

    z85_encode(
        reinterpret_cast<const unsigned char*>(array.data()),
        array_size_in_bytes,
        &encoded[0]);

    return encoded;
}

Array z85_decode(const ArrayType type, const std::string& data)
{
    Array array(type);
    const size_t decoded_size = z85_decoded_size(data.size());
    const size_t size = decoded_size / array.item_size();

    array.resize(size);
    z85_decode(data.c_str(), data.size(), reinterpret_cast<unsigned char*>(array.data()));
    return array;
}

std::string z85_encode(const KeyFramedArray& array)
{
    const Array& a = array.get_key(0);
    const size_t array_size_in_bytes = a.size() * a.item_size();
    assert(array_size_in_bytes % 4 == 0);

    const size_t encoded_size = z85_encoded_size(array_size_in_bytes * array.get_key_count());

    std::string encoded;
    encoded.resize(encoded_size);

    auto* p = &encoded[0];
    for (size_t i = 0, e = array.get_key_count(); i < e; ++i)
    {
        z85_encode(
            reinterpret_cast<const unsigned char*>(array.get_key(i).data()),
            array_size_in_bytes,
            p);

        p += array_size_in_bytes;
    }

    return encoded;
}

KeyFramedArray z85_decode(
    const ArrayType     type,
    const size_t        size,
    const size_t        keys,
    const std::string&  data)
{
    KeyFramedArray array(type, size, keys);

    const size_t keyframe_size = data.size() / keys;
    const auto* p = data.c_str();

    for (size_t i = 0; i < keys; ++i)
    {
        z85_decode(
            p,
            keyframe_size,
            reinterpret_cast<unsigned char*>(array.get_key(i).data()));
        p += keyframe_size;
    }

    return array;
}

}   // namespace foundation
