/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/renderer/speedreader_js_handler.h"

#include "brave/components/speedreader/common/constants.h"
#include "brave/components/speedreader/common/speedreader.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace {
constexpr const char kSpeedreader[] = "speedreader";
}

namespace speedreader {

gin::WrapperInfo SpeedreaderJSHandler::kWrapperInfo = {gin::kEmbedderNativeGin};

SpeedreaderJSHandler::SpeedreaderJSHandler(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}

SpeedreaderJSHandler::~SpeedreaderJSHandler() = default;

// static
void SpeedreaderJSHandler::Install(content::RenderFrame* render_frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context =
      render_frame->GetWebFrame()->GetScriptContextFromWorldId(
          isolate, kIsolatedWorldId);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> global = context->Global();

  // check object existence
  v8::Local<v8::Value> speedreader_value =
      global->Get(context, gin::StringToV8(isolate, kSpeedreader))
          .ToLocalChecked();
  if (!speedreader_value->IsUndefined())
    return;

  gin::Handle<SpeedreaderJSHandler> handler =
      gin::CreateHandle(isolate, new SpeedreaderJSHandler(render_frame));
  if (handler.IsEmpty())
    return;

  v8::PropertyDescriptor desc(handler.ToV8(), false);
  desc.set_configurable(false);

  global
      ->DefineProperty(isolate->GetCurrentContext(),
                       gin::StringToV8(isolate, kSpeedreader), desc)
      .Check();
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
