/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_js_handler.h"

#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated_map.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "ui/base/resource/resource_bundle.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace {

static base::NoDestructor<std::string> g_provider_script("");

std::string LoadDataResource(const int id) {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    return resource_bundle.LoadDataResourceString(id);
  }

  return resource_bundle.GetRawDataResource(id).as_string();
}

}  // namespace

namespace brave_wallet {

BraveWalletJSHandler::BraveWalletJSHandler(
    content::RenderFrame* render_frame)
    : render_frame_(render_frame) {
  if (g_provider_script->empty()) {
    *g_provider_script = LoadDataResource(kBraveWalletScriptGenerated[0].value);
  }
  EnsureConnected();
}

BraveWalletJSHandler::~BraveWalletJSHandler() = default;

bool BraveWalletJSHandler::EnsureConnected() {
  if (!brave_wallet_provider_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        brave_wallet_provider_.BindNewPipeAndPassReceiver());
  }

  return brave_wallet_provider_.is_bound();
}

void BraveWalletJSHandler::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  CreateWorkerObject(isolate, context);
}

void BraveWalletJSHandler::CreateWorkerObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> provider_obj;
  v8::Local<v8::Value> provider_value;
  if (!global->Get(context, gin::StringToV8(isolate, "brave_provider_handler"))
           .ToLocal(&provider_value) ||
      !provider_value->IsObject()) {
    provider_obj = v8::Object::New(isolate);
    global
        ->Set(context, gin::StringToSymbol(isolate, "brave_provider_handler"),
              provider_obj)
        .Check();
    BindFunctionsToObject(isolate, provider_obj);
  }
}

void BraveWalletJSHandler::BindFunctionsToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> javascript_object) {
  BindFunctionToObject(
      isolate, javascript_object, "request",
      base::BindRepeating(&BraveWalletJSHandler::Request,
                          base::Unretained(this)));
}

template <typename Sig>
void BraveWalletJSHandler::BindFunctionToObject(
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

void BraveWalletJSHandler::Request(const std::string& input) {
  if (!EnsureConnected())
    return;

  brave_wallet_provider_->Request(
    input,
    base::BindOnce(&BraveWalletJSHandler::OnRequest,
                   base::Unretained(this)));
}

void BraveWalletJSHandler::OnRequest(const int status,
                                     const std::string& response) {
}

void BraveWalletJSHandler::InjectScript() {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  if (web_frame->IsProvisional())
    return;

  web_frame->ExecuteScript(blink::WebString::FromUTF8(*g_provider_script));
}

}  // namespace brave_wallet
