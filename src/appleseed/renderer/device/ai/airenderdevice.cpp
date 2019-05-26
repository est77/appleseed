
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
#include "airenderdevice.h"

// appleseed.renderer headers.
#include "renderer/modeling/project/project.h"
#include "renderer/modeling/camera/perspectivecamera.h"
#include "renderer/modeling/scene/scene.h"
#include "renderer/modeling/frame/frame.h"
#include "renderer/kernel/rendering/itilecallback.h"

// appleseed.foundation headers.
#include "foundation/image/image.h"
#include "foundation/platform/thread.h"
#include "foundation/utility/containers/dictionary.h"

// Arnold headers.
#include "ai.h"

// standard headers.
#include <string>

using namespace foundation;
using namespace std;

// Defined in driver.cpp
extern const AtNodeMethods* as_ai_driver;

namespace renderer
{

struct AiRenderDevice::Impl
{
    AiRenderContext m_context;
    AtNode*         m_driver = nullptr;
};

AiRenderDevice::AiRenderDevice(
    Project&                project,
    const ParamArray&       params)
  : RenderDeviceBase(project, params)
  , mImpl(new Impl())
{
}

AiRenderDevice::~AiRenderDevice()
{
    delete mImpl;
}

bool AiRenderDevice::initialize(
    const SearchPaths&      resource_searchpaths,
    ITileCallbackFactory*   tile_callback_factory,
    IAbortSwitch&           abort_switch)
{
    if (is_progressive_render())
    {
        RENDERER_LOG_ERROR("Arnold render device does not support progressive rendering");
        return false;
    }

    if (AiUniverseIsActive())
        uninitialize();

    AiBegin();

    // Convert render settings options.
    AtNode* options = AiUniverseGetOptions();

    // Image size.
    auto* frame = get_project().get_frame();
    const auto& props = frame->image().properties();
    AiNodeSetInt(options, AtString("xres"), static_cast<int>(props.m_canvas_width));
    AiNodeSetInt(options, AtString("yres"), static_cast<int>(props.m_canvas_height));

    // Bucket size.
    if (props.m_tile_width != props.m_tile_height)
    {
        RENDERER_LOG_ERROR("Non square tile sizes not supported");
        return false;
    }

    AiNodeSetInt(options, AtString("bucket_size"), static_cast<int>(props.m_tile_width));

    // Crop window.
    if (frame->has_crop_window())
    {
        const auto crop = frame->get_crop_window();
        AiNodeSetInt(options, AtString("region_min_x"), static_cast<int>(crop.min.x));
        AiNodeSetInt(options, AtString("region_min_y"), static_cast<int>(crop.min.y));
        AiNodeSetInt(options, AtString("region_max_x"), static_cast<int>(crop.max.x));
        AiNodeSetInt(options, AtString("region_max_y"), static_cast<int>(crop.max.y));
    }

    // Install our driver node.
    AiNodeEntryInstall(
        AI_NODE_DRIVER,
        AI_TYPE_RGBA,
        "as_ai_driver",
        "<built-in>",
        as_ai_driver,
        AI_VERSION);

    // Instance our driver.
    mImpl->m_driver = AiNode("as_ai_driver");
    AiNodeSetStr(mImpl->m_driver, "name", "as_driver");
    AiNodeSetPtr(mImpl->m_driver, "frame", frame);

    // Create a pixel filter.
    AtNode *filter = AiNode("gaussian_filter");
    AiNodeSetStr(filter, "name", "gauss_filter");

    // Add our outputs.
    AtArray *outputs_array = AiArrayAllocate(1, 1, AI_TYPE_STRING);
    AiArraySetStr(outputs_array, 0, "RGBA RGBA gauss_filter as_driver");
    AiNodeSetArray(options, "outputs", outputs_array);

    return true;
}

void AiRenderDevice::uninitialize()
{
    AiNodeEntryUninstall("as_ai_driver");
    AiEnd();
}

bool AiRenderDevice::build_or_update_scene()
{
    // todo: implement me...
    return true;
}

bool AiRenderDevice::load_checkpoint(Frame& frame, const size_t pass_count)
{
    return true;
}

bool AiRenderDevice::on_render_begin(
    OnRenderBeginRecorder&  recorder,
    IAbortSwitch*           abort_switch)
{
    return true;
}

bool AiRenderDevice::on_frame_begin(
    OnFrameBeginRecorder&   recorder,
    IAbortSwitch*           abort_switch)
{
    return true;
}

const IRenderContext& AiRenderDevice::get_render_context() const
{
    return mImpl->m_context;
}

IRendererController::Status AiRenderDevice::render_frame(
    ITileCallbackFactory*   tile_callback_factory,
    IRendererController&    renderer_controller,
    IAbortSwitch&           abort_switch)
{
    // Create a tile callback and pass it to our driver.
    std::unique_ptr<ITileCallback> tile_callback(
        tile_callback_factory->create());
    AiNodeSetPtr(mImpl->m_driver, "tile_callback", tile_callback.get());

    #ifndef NDEBUG
        AiASSWrite("/tmp/as.ass", AI_NODE_ALL, false, false);
    #endif

    // Launch render.
    IRendererController::Status result;
    AiRenderBegin();

    while (true)
    {
        const auto status = AiRenderGetStatus();

        if (status == AI_RENDER_STATUS_FINISHED)
        {
            result = IRendererController::TerminateRendering;
            break;
        }
        else if (status == AI_RENDER_STATUS_FAILED)
        {
            result = IRendererController::TerminateRendering;
            break;
        }

        if (abort_switch.is_aborted())
        {
            result = IRendererController::TerminateRendering;
            AiRenderAbort(AI_BLOCKING);
            break;
        }

        renderer_controller.on_progress();
        foundation::sleep(1);   // namespace qualifer required
    }

    AiRenderEnd();
    uninitialize();
    return result;
}

void AiRenderDevice::print_settings() const
{
}

Dictionary AiRenderDevice::get_metadata()
{
    Dictionary metadata;
    return metadata;
}

}
