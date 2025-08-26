/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/renderer/speedreader_js_handler.h"

#include "base/check.h"
#include "brave/components/speedreader/common/speedreader.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-cppgc.h"

namespace {
constexpr const char kSpeedreader[] = "speedreader";
}

namespace speedreader {

SpeedreaderJSHandler::SpeedreaderJSHandler(content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {
  self_ = this;
}

SpeedreaderJSHandler::~SpeedreaderJSHandler() = default;

void SpeedreaderJSHandler::OnDestruct() {
  self_.Clear();
}

// static
void SpeedreaderJSHandler::Install(content::RenderFrame* render_frame,
                                   v8::Local<v8::Context> context) {
  CHECK(render_frame);
  v8::Isolate* isolate =
      render_frame->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);

  if (context.IsEmpty()) {
    return;
  }

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> global = context->Global();

  // check object existence
  v8::Local<v8::Value> speedreader_value =
      global->Get(context, gin::StringToV8(isolate, kSpeedreader))
          .ToLocalChecked();
  if (!speedreader_value->IsUndefined()) {
    return;
  }

  SpeedreaderJSHandler* handler =
      cppgc::MakeGarbageCollected<SpeedreaderJSHandler>(
          isolate->GetCppHeap()->GetAllocationHandle(), render_frame);

  v8::PropertyDescriptor desc(handler->GetWrapper(isolate).ToLocalChecked(),
                              false);
  desc.set_configurable(false);

  global
      ->DefineProperty(isolate->GetCurrentContext(),
                       gin::StringToV8(isolate, kSpeedreader), desc)
      .Check();
}

gin::ObjectTemplateBuilder SpeedreaderJSHandler::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<SpeedreaderJSHandler>::GetObjectTemplateBuilder(isolate)
      .SetMethod("showOriginalPage", &SpeedreaderJSHandler::ShowOriginalPage)
      .SetMethod("ttsPlayPause", &SpeedreaderJSHandler::TtsPlayPause);
}

void SpeedreaderJSHandler::ShowOriginalPage(v8::Isolate* isolate) {
  DCHECK(isolate);
  if (!render_frame()) {
    return;
  }

  mojo::AssociatedRemote<speedreader::mojom::SpeedreaderHost> speedreader_host;
  render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(
      &speedreader_host);

  if (speedreader_host.is_bound()) {
    speedreader_host->OnShowOriginalPage();
  }
}

const gin::WrapperInfo* SpeedreaderJSHandler::wrapper_info() const {
  return &kWrapperInfo;
}

void SpeedreaderJSHandler::TtsPlayPause(v8::Isolate* isolate,
                                        int paragraph_index) {
  DCHECK(isolate);
  if (!render_frame()) {
    return;
  }

  mojo::AssociatedRemote<speedreader::mojom::SpeedreaderHost> speedreader_host;
  render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(
      &speedreader_host);

  if (speedreader_host.is_bound()) {
    speedreader_host->OnTtsPlayPause(paragraph_index);
  }
}

}  // namespace speedreader
