/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_RENDERER_PSST_JS_HANDLER_H_
#define BRAVE_COMPONENTS_PSST_RENDERER_PSST_JS_HANDLER_H_

#include "brave/components/psst/core/common/psst_ui_common.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace psst {

// Installs the diagnostics logging API used by the injected PSST user and
// policy scripts, and forwards what they log to the browser.
//
// See the "Script logging" section of components/psst/README.md for the
// script-facing contract.
class PsstJSHandler final : public gin::Wrappable<PsstJSHandler>,
                            public content::RenderFrameObserver {
 public:
  static constexpr gin::WrapperInfo kWrapperInfo = {{gin::kEmbedderNativeGin},
                                                    gin::kPsstBindings};

  explicit PsstJSHandler(content::RenderFrame* render_frame);
  PsstJSHandler(const PsstJSHandler&) = delete;
  PsstJSHandler& operator=(const PsstJSHandler&) = delete;
  ~PsstJSHandler() final;

  // Installs `psstLog()` and the `console.*` forwarding shim into `context`.
  // Must only be called for the PSST isolated world, so that page script can
  // neither call nor spoof them.
  static void Install(content::RenderFrame* render_frame,
                      v8::Local<v8::Context> context);

  // content::RenderFrameObserver:
  // Empty: this object is owned by the V8 heap, not by the render frame.
  void OnDestruct() override;

 private:
  // gin::WrappableBase:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;
  const gin::WrapperInfo* wrapper_info() const override;

  // Called from JS as `psstLog(level, message, data)`.
  void PsstLog(gin::Arguments* args);

  mojo::AssociatedRemote<mojom::PsstLogSink>& GetLogSink();

  mojo::AssociatedRemote<mojom::PsstLogSink> log_sink_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_RENDERER_PSST_JS_HANDLER_H_
