/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/renderer/speedreader_js_handler.h"

#include <tuple>
#include <utility>

#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace speedreader {

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
  BindFunctionsToObject(isolate, context);
}

void SpeedreaderJSHandler::ResetRemote(content::RenderFrame* render_frame) {
  render_frame_ = render_frame;
  speedreader_host_.reset();
  EnsureConnected();
}

bool SpeedreaderJSHandler::EnsureConnected() {
  if (!speedreader_host_.is_bound()) {
    render_frame_->GetRemoteAssociatedInterfaces()->GetInterface(
        &speedreader_host_);
  }
  return speedreader_host_.is_bound();
}

void SpeedreaderJSHandler::BindFunctionsToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> speedreader_obj;
  v8::Local<v8::Value> brave_value;
  if (!global->Get(context, gin::StringToV8(isolate, "speedreader"))
           .ToLocal(&brave_value) ||
      !brave_value->IsObject()) {
    speedreader_obj = v8::Object::New(isolate);
    global
        ->Set(context, gin::StringToSymbol(isolate, "speedreader"),
              speedreader_obj)
        .Check();
  } else {
    speedreader_obj = brave_value->ToObject(context).ToLocalChecked();
  }

  speedreader_obj
      ->Set(context, gin::StringToSymbol(isolate, "showOriginalPage"),
            gin::CreateFunctionTemplate(
                isolate,
                base::BindRepeating(&SpeedreaderJSHandler::ShowOriginalPage,
                                    base::Unretained(this), isolate))
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

void SpeedreaderJSHandler::ShowOriginalPage(v8::Isolate* isolate) {
  DCHECK(isolate);
  if (!EnsureConnected())
    return;

  speedreader_host_->OnShowOriginalPage();
}

}  // namespace speedreader
