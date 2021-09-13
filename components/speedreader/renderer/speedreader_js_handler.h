/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_HANDLER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/speedreader/common/speedreader_ui.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "v8/include/v8.h"

namespace speedreader {

class SpeedreaderJsHandler : public mojom::SpeedreaderUIObserver {
 public:
  explicit SpeedreaderJsHandler(content::RenderFrame* render_frame,
                                const int32_t isolated_world_id);
  ~SpeedreaderJsHandler() override;

 public:
  void EnsureConnected();

  // mojom::SpeedreaderUIObserver:
  void OnFontScaleChanged(float scale) override;

  content::RenderFrame* render_frame_;
  int32_t isolated_world_id_;

  // UI service interface
  mojo::Receiver<mojom::SpeedreaderUIObserver> receiver_{this};
  mojo::Remote<mojom::SpeedreaderUI> remote_;

  base::WeakPtrFactory<SpeedreaderJsHandler> weak_ptr_factory_{this};
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_HANDLER_H_
