/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_RENDERER_PSST_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_PSST_RENDERER_PSST_RENDER_FRAME_OBSERVER_H_

#include <cstdint>

#include "content/public/renderer/render_frame_observer.h"

namespace psst {

// Installs the PSST script logging API into the isolated world the PSST user
// and policy scripts run in, and nowhere else.
class PsstRenderFrameObserver : public content::RenderFrameObserver {
 public:
  PsstRenderFrameObserver(content::RenderFrame* render_frame,
                          int32_t isolated_world_id);
  PsstRenderFrameObserver(const PsstRenderFrameObserver&) = delete;
  PsstRenderFrameObserver& operator=(const PsstRenderFrameObserver&) = delete;
  ~PsstRenderFrameObserver() override;

  // content::RenderFrameObserver:
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;

 private:
  // content::RenderFrameObserver:
  void OnDestruct() override;

  const int32_t isolated_world_id_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_RENDERER_PSST_RENDER_FRAME_OBSERVER_H_
