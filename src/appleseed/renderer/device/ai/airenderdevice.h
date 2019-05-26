
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

// appleseed.renderer headers.
#include "renderer/device/irendercontext.h"
#include "renderer/device/renderdevicebase.h"

// Forward declarations.
namespace foundation    { class Dictionary; }

namespace renderer
{

class AiRenderContext
  : public IRenderContext
{
};

class AiRenderDevice
  : public RenderDeviceBase
{
  public:
    AiRenderDevice(
        Project&                        project,
        const ParamArray&               params);

    ~AiRenderDevice() override;

    bool initialize(
        const foundation::SearchPaths&  resource_search_paths,
        ITileCallbackFactory*           tile_callback_factory,
        foundation::IAbortSwitch&       abort_switch) override;

    bool build_or_update_scene() override;

    bool load_checkpoint(Frame& frame, const size_t pass_count) override;

    bool on_render_begin(
        OnRenderBeginRecorder&          recorder,
        foundation::IAbortSwitch*       abort_switch = nullptr) override;

    bool on_frame_begin(
        OnFrameBeginRecorder&           recorder,
        foundation::IAbortSwitch*       abort_switch = nullptr) override;

    const IRenderContext& get_render_context() const override;

    IRendererController::Status render_frame(
        ITileCallbackFactory*           tile_callback_factory,
        IRendererController&            renderer_controller,
        foundation::IAbortSwitch&       abort_switch) override;

    void print_settings() const override;

    // Return device metadata.
    static foundation::Dictionary get_metadata();

  private:
    struct Impl;
    Impl* mImpl;

    void uninitialize();
};

}       // namespace renderer
