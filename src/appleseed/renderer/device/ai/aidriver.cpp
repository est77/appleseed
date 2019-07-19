
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

// appleseed.renderer headers.
#include "renderer/kernel/rendering/itilecallback.h"
#include "renderer/modeling/frame/frame.h"

// appleseed.foundation headers.
#include "foundation/image/image.h"

// Arnold headers.
#include "ai.h"

using namespace renderer;
using namespace foundation;

namespace
{

struct DriverData
{
    Frame*          m_frame = nullptr;
    ITileCallback*  m_tile_callback = nullptr;
    size_t          m_tile_size = 0;
};

}

AI_DRIVER_NODE_EXPORT_METHODS(as_ai_driver);

static void Initialize(AtNode* node)
{
    AiDriverInitialize(node, false);
    AiNodeSetLocalData(node, new DriverData);
}

static void Parameters(AtList* params, AtNodeEntry* nentry)
{
    AiParameterPtr("frame", nullptr);
    AiParameterPtr("tile_callback", nullptr);
}

static void Update(AtNode* node)
{
}

static void Finish(AtNode* node)
{
    delete reinterpret_cast<DriverData*>(AiNodeGetLocalData(node));
}

static bool DriverSupportsPixelType(const AtNode* node, uint8_t pixel_type)
{
    if (pixel_type == AI_TYPE_RGB || pixel_type == AI_TYPE_RGBA)
        return true;

    return false;
}

static const char** DriverExtension()
{
    return nullptr;
}

static void DriverOpen(AtNode* node, struct AtOutputIterator* iterator, AtBBox2 display_window, AtBBox2 data_window, int bucket_size)
{
    auto* data = reinterpret_cast<DriverData*>(AiNodeGetLocalData(node));
    data->m_frame = reinterpret_cast<Frame*>(AiNodeGetPtr(node, "frame"));
    data->m_tile_callback = reinterpret_cast<ITileCallback*>(AiNodeGetPtr(node, "tile_callback"));

    const auto& props = data->m_frame->image().properties();
    data->m_tile_size = props.m_tile_width;
}

static bool DriverNeedsBucket(AtNode* node, int bucket_xo, int bucket_yo, int bucket_size_x, int bucket_size_y, uint16_t tid)
{
    return true;
}

static void DriverPrepareBucket(AtNode* node, int bucket_xo, int bucket_yo, int bucket_size_x, int bucket_size_y, uint16_t tid)
{
    auto* data = reinterpret_cast<DriverData*>(AiNodeGetLocalData(node));
    data->m_tile_callback->on_tile_begin(
        data->m_frame,
        bucket_xo / data->m_tile_size,
        bucket_yo / data->m_tile_size);
}

static void DriverProcessBucket(AtNode* node, struct AtOutputIterator* iterator, struct AtAOVSampleIterator* sample_iterator, int bucket_xo, int bucket_yo, int bucket_size_x, int bucket_size_y, uint16_t tid)
{
}

static void DriverWriteBucket(AtNode* node, struct AtOutputIterator* iterator, struct AtAOVSampleIterator* sample_iterator, int bucket_xo, int bucket_yo, int bucket_size_x, int bucket_size_y)
{
    int pixel_type;
    const void* bucket_data;
    if (!AiOutputIteratorGetNext(iterator, nullptr, &pixel_type, &bucket_data))
        return;

    // Copy the pixels;
    const auto* data = reinterpret_cast<DriverData*>(AiNodeGetLocalData(node));
    auto& image = data->m_frame->image();

    const float* p = reinterpret_cast<const float*>(bucket_data);
    for (size_t j = 0; j < bucket_size_y; ++j)
    {
        for (size_t i = 0; i < bucket_size_x; ++i)
        {
            image.set_pixel(
                bucket_xo + i,
                bucket_yo + j,
                p,
                4);
            p += 4;
        }
    }

    // Call the tile callback.
    data->m_tile_callback->on_tile_end(
        data->m_frame,
        bucket_xo / data->m_tile_size,
        bucket_yo / data->m_tile_size);
}

static void DriverClose(AtNode* node, struct AtOutputIterator* iterator)
{
}
