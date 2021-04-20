/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_js_handler.h"

#include <utility>

#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/renderer/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/renderer/web3_provider_constants.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"


namespace brave_wallet {

BraveWalletJSHandler::BraveWalletJSHandler(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}

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

  CreateEthereumObject(isolate, context);
}

void BraveWalletJSHandler::CreateEthereumObject(
    v8::Isolate* isolate, v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> cosmetic_filters_obj;
  v8::Local<v8::Value> cosmetic_filters_value;
  if (!global->Get(context, gin::StringToV8(isolate, "ethereum"))
           .ToLocal(&cosmetic_filters_value) ||
      !cosmetic_filters_value->IsObject()) {
    cosmetic_filters_obj = v8::Object::New(isolate);
    global
        ->Set(context, gin::StringToSymbol(isolate, "ethereum"),
              cosmetic_filters_obj)
        .Check();
    BindFunctionsToObject(isolate, context, cosmetic_filters_obj);
  }
}

void BraveWalletJSHandler::BindFunctionsToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> javascript_object) {
  BindFunctionToObject(isolate, javascript_object, "request",
                       base::BindRepeating(&BraveWalletJSHandler::Request,
                                           base::Unretained(this), isolate));
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

v8::Local<v8::Promise> BraveWalletJSHandler::Request(v8::Isolate* isolate,
                                                     const std::string& input) {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();

  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (!resolver.IsEmpty()) {
    auto promise_resolver(
        v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
    auto context_old(
        v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
    brave_wallet_provider_->Request(
        input,
        base::BindOnce(&BraveWalletJSHandler::OnRequest, base::Unretained(this),
                       std::move(promise_resolver), isolate,
                       std::move(context_old)));

    return resolver.ToLocalChecked()->GetPromise();
  }

  return v8::Local<v8::Promise>();
}

void BraveWalletJSHandler::OnRequest(
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Global<v8::Context> context_old,
    const int http_code,
    const std::string& response) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  v8::Local<v8::String> result;
  bool reject = http_code != 200;
  ProviderErrors code = ProviderErrors::kDisconnected;
  std::string message;
  std::string formed_response;
  if (reject) {
    code = ProviderErrors::kUnsupportedMethod;
    message = "HTTP Status code: " + base::NumberToString(http_code);
    formed_response = FormProviderResponse(code, message);
  } else {
    formed_response = FormProviderResponse(response, &reject);
  }
  result = v8::String::NewFromUtf8(isolate, formed_response.c_str())
               .ToLocalChecked();

  if (reject) {
    ALLOW_UNUSED_LOCAL(resolver->Reject(context, result));
  } else {
    ALLOW_UNUSED_LOCAL(resolver->Resolve(context, result));
  }
}

}  // namespace brave_wallet
