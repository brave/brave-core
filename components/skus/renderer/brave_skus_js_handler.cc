/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/renderer/brave_skus_js_handler.h"

#include <utility>

#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace brave_rewards {

BraveSkusJSHandler::BraveSkusJSHandler(
    content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}

BraveSkusJSHandler::~BraveSkusJSHandler() = default;

bool BraveSkusJSHandler::EnsureConnected() {
  if (!skus_sdk_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        skus_sdk_.BindNewPipeAndPassReceiver());
  }

  return skus_sdk_.is_bound();
}

void BraveSkusJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  BindFunctionsToObject(isolate, context);
}

void BraveSkusJSHandler::ResetRemote(
    content::RenderFrame* render_frame) {
  render_frame_ = render_frame;
  skus_sdk_.reset();
  EnsureConnected();
}

void BraveSkusJSHandler::BindFunctionsToObject(
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
      isolate, brave_obj, "refresh_order",
      base::BindRepeating(
          &BraveSkusJSHandler::RefreshOrder,
          base::Unretained(this), isolate));
}

template <typename Sig>
void BraveSkusJSHandler::BindFunctionToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> javascript_object,
    const std::string& name,
    const base::RepeatingCallback<Sig>& callback) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  javascript_object
      ->Set(context, gin::StringToSymbol(isolate, name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}


void BraveSkusJSHandler::RefreshOrder(
    v8::Isolate* isolate) {
  if (!EnsureConnected())
    return;
  auto* web_frame = render_frame_->GetWebFrame();
  // Prevent site from calling this in response to a DOM event or Timer.
  if (web_frame->HasTransientUserActivation()) {
    // TODO: get real argument
    skus_sdk_->StartRefreshOrder(123);
  } else {
    blink::WebString message =
        "setIsDefaultSearchProvider: "
        "API can only be initiated by a user gesture.";
    web_frame->AddMessageToConsole(blink::WebConsoleMessage(
        blink::mojom::ConsoleMessageLevel::kWarning, message));
  }
}

}  // namespace brave_rewards
