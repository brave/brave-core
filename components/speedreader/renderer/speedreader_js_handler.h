// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_HANDLER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_HANDLER_H_

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/wrappable.h"

namespace speedreader {

class SpeedreaderJSHandler final : public gin::Wrappable<SpeedreaderJSHandler>,
                                   public content::RenderFrameObserver {
 public:
  static constexpr gin::WrapperInfo kWrapperInfo = {{gin::kEmbedderNativeGin},
                                                    gin::SpeedreaderBindings};

  explicit SpeedreaderJSHandler(content::RenderFrame* render_frame);
  ~SpeedreaderJSHandler() final;
  SpeedreaderJSHandler(const SpeedreaderJSHandler&) = delete;
  SpeedreaderJSHandler& operator=(const SpeedreaderJSHandler&) = delete;

  static void Install(content::RenderFrame* render_frame,
                      v8::Local<v8::Context> context);

  // content::RenderFrameObserver:
  void OnDestruct() override;

 private:

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;
  const gin::WrapperInfo* wrapper_info() const override;

  // A function to be called from JS
  void ShowOriginalPage(v8::Isolate* isolate);

  void TtsPlayPause(v8::Isolate* isolate, int index);
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_HANDLER_H_
