/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/youtube_script_injector/renderer/youtube_injector_frame_js_handler.h"

#include <tuple>
#include <utility>

#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace youtube_script_injector {

YouTubeInjectorFrameJSHandler::YouTubeInjectorFrameJSHandler(
    content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}

YouTubeInjectorFrameJSHandler::~YouTubeInjectorFrameJSHandler() = default;

bool YouTubeInjectorFrameJSHandler::EnsureConnected() {
  if (!youtube_injector_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker().GetInterface(
        youtube_injector_.BindNewPipeAndPassReceiver());
  }

  return youtube_injector_.is_bound();
}

void YouTubeInjectorFrameJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  CHECK(render_frame_);
  v8::Isolate* isolate =
      render_frame_->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty()) {
    return;
  }

  v8::Context::Scope context_scope(context);

  BindFunctionsToObject(isolate, context);
}

void YouTubeInjectorFrameJSHandler::ResetRemote(
    content::RenderFrame* render_frame) {
  render_frame_ = render_frame;
  youtube_injector_.reset();
  EnsureConnected();
}

void YouTubeInjectorFrameJSHandler::BindFunctionsToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> brave_obj;
  v8::Local<v8::Value> brave_value;
  if (!global->Get(context, gin::StringToV8(isolate, "brave"))
           .ToLocal(&brave_value) ||
      !brave_value->IsObject()) {
    brave_obj = v8::Object::New(isolate);
    global->Set(context, gin::StringToSymbol(isolate, "brave"), brave_obj)
        .Check();
  } else {
    brave_obj = brave_value->ToObject(context).ToLocalChecked();
  }
  BindFunctionToObject(
      isolate, brave_obj, "nativePipMode",
      base::BindRepeating(&YouTubeInjectorFrameJSHandler::NativePipMode,
                          base::Unretained(this)));
}

template <typename Sig>
void YouTubeInjectorFrameJSHandler::BindFunctionToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> javascript_object,
    const std::string& name,
    const base::RepeatingCallback<Sig>& callback) {
  LOG(ERROR) << "SIMONE - BindFunctionToObject";
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  javascript_object
      ->Set(context, gin::StringToSymbol(isolate, name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

void YouTubeInjectorFrameJSHandler::NativePipMode() {
  if (!EnsureConnected()) {
    return;
  }
  // auto* web_frame = render_frame_->GetWebFrame();
  youtube_injector_->NativePipMode();
}

}  // namespace youtube_script_injector
