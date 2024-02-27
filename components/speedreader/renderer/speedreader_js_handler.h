// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_HANDLER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame.h"
#include "gin/wrappable.h"

namespace speedreader {

class SpeedreaderRenderFrameObserver;

class SpeedreaderJSHandler final : public gin::Wrappable<SpeedreaderJSHandler> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  SpeedreaderJSHandler(const SpeedreaderJSHandler&) = delete;
  SpeedreaderJSHandler& operator=(const SpeedreaderJSHandler&) = delete;

  static void Install(base::WeakPtr<SpeedreaderRenderFrameObserver> owner,
                      v8::Local<v8::Context> context);

 private:
  explicit SpeedreaderJSHandler(
      base::WeakPtr<SpeedreaderRenderFrameObserver> owner);
  ~SpeedreaderJSHandler() final;

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;

  // A function to be called from JS
  void ShowOriginalPage(v8::Isolate* isolate);

  void TtsPlayPause(v8::Isolate* isolate, int index);

  base::WeakPtr<SpeedreaderRenderFrameObserver> owner_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_RENDERER_SPEEDREADER_JS_HANDLER_H_
