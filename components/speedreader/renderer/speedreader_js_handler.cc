/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/renderer/speedreader_js_handler.h"

#include "brave/components/speedreader/common/speedreader.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/web/blink.h"

namespace speedreader {

gin::WrapperInfo SpeedreaderJSHandler::kWrapperInfo = {gin::kEmbedderNativeGin};

SpeedreaderJSHandler::SpeedreaderJSHandler(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}

SpeedreaderJSHandler::~SpeedreaderJSHandler() = default;

void SpeedreaderJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> global = context->Global();

  gin::Handle<SpeedreaderJSHandler> handler = gin::CreateHandle(isolate, this);
  if (handler.IsEmpty())
    return;

  v8::PropertyDescriptor desc(handler.ToV8(), false);
  desc.set_configurable(false);

  std::ignore =
      global->DefineProperty(isolate->GetCurrentContext(),
                             gin::StringToV8(isolate, "speedreader"), desc);
}

void SpeedreaderJSHandler::ResetRemote(content::RenderFrame* render_frame) {
  render_frame_ = render_frame;
}

gin::ObjectTemplateBuilder SpeedreaderJSHandler::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<SpeedreaderJSHandler>::GetObjectTemplateBuilder(isolate)
      .SetMethod("showOriginalPage", &SpeedreaderJSHandler::ShowOriginalPage);
}

void SpeedreaderJSHandler::ShowOriginalPage(v8::Isolate* isolate) {
  DCHECK(isolate);
  mojo::AssociatedRemote<speedreader::mojom::SpeedreaderHost> speedreader_host;
  render_frame_->GetRemoteAssociatedInterfaces()->GetInterface(
      &speedreader_host);

  if (speedreader_host.is_bound()) {
    speedreader_host->OnShowOriginalPage();
  }
}

}  // namespace speedreader
