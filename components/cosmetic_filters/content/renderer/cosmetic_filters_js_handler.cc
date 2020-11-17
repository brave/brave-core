/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/cosmetic_filters/content/renderer/cosmetic_filters_js_handler.h"

#include <string>

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "v8/include/v8.h"

namespace cosmetic_filters_worker {

CosmeticFiltersJSHandler::CosmeticFiltersJSHandler(
    content::RenderFrame* render_frame):
    render_frame_(render_frame) {}

CosmeticFiltersJSHandler::~CosmeticFiltersJSHandler() = default;

void CosmeticFiltersJSHandler::HiddenClassIdSelectors(
    const std::string& input) {
  if (cs_communicator_) {
    cs_communicator_->HiddenClassIdSelectors(input);
  }
}

void CosmeticFiltersJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> distiller_obj =
      GetOrCreateWorkerObject(isolate, context);

  EnsureConnected();

  BindFunctionToObject(
      isolate, distiller_obj, "hiddenClassIdSelectors",
      base::BindRepeating(&CosmeticFiltersJSHandler::HiddenClassIdSelectors,
                          base::Unretained(this)));
}

template <typename Sig>
void CosmeticFiltersJSHandler::BindFunctionToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> javascript_object,
    const std::string& name,
    const base::RepeatingCallback<Sig>& callback) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  // Get the isolate associated with this object.
  javascript_object
      ->Set(context, gin::StringToSymbol(isolate, name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

void CosmeticFiltersJSHandler::EnsureConnected() {
  if (!cs_communicator_) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        cs_communicator_.BindNewPipeAndPassReceiver());
  }
}

v8::Local<v8::Object> GetOrCreateWorkerObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> distiller_obj;
  v8::Local<v8::Value> distiller_value;
  if (!global->Get(context, gin::StringToV8(isolate, "cf_worker"))
           .ToLocal(&distiller_value) ||
      !distiller_value->IsObject()) {
    distiller_obj = v8::Object::New(isolate);
    global
        ->Set(context, gin::StringToSymbol(isolate, "cf_worker"), distiller_obj)
        .Check();
  } else {
    distiller_obj = v8::Local<v8::Object>::Cast(distiller_value);
  }
  return distiller_obj;
}

}  // namespace cosmetic_filters_worker
