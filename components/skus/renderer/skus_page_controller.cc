/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/renderer/skus_page_controller.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace skus {

SkusPageController::SkusPageController(content::RenderFrame* render_frame)
    : render_frame_(render_frame) {}

SkusPageController::~SkusPageController() = default;

bool SkusPageController::EnsureConnected() {
  if (!skus_service_.is_bound()) {
    render_frame_->GetBrowserInterfaceBroker()->GetInterface(
        skus_service_.BindNewPipeAndPassReceiver());
  }

  return skus_service_.is_bound();
}

void SkusPageController::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  BindFunctionsToObject(isolate, context);
}

void SkusPageController::ResetRemote(content::RenderFrame* render_frame) {
  render_frame_ = render_frame;
  skus_service_.reset();
  EnsureConnected();
}

void SkusPageController::BindFunctionsToObject(v8::Isolate* isolate,
                                               v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();

  // window.chrome
  v8::Local<v8::Object> chrome_obj;
  v8::Local<v8::Value> chrome_value;
  if (!global->Get(context, gin::StringToV8(isolate, "chrome"))
           .ToLocal(&chrome_value) ||
      !chrome_value->IsObject()) {
    chrome_obj = v8::Object::New(isolate);
    global->Set(context, gin::StringToSymbol(isolate, "chrome"), chrome_obj)
        .Check();
  } else {
    chrome_obj = chrome_value->ToObject(context).ToLocalChecked();
  }

  // window.chrome.braveSkus
  v8::Local<v8::Object> skus_obj;
  v8::Local<v8::Value> skus_value;
  if (!chrome_obj->Get(context, gin::StringToV8(isolate, "braveSkus"))
           .ToLocal(&skus_value) ||
      !skus_value->IsObject()) {
    skus_obj = v8::Object::New(isolate);
    chrome_obj
        ->Set(context, gin::StringToSymbol(isolate, "braveSkus"), skus_obj)
        .Check();
  } else {
    skus_obj = skus_value->ToObject(context).ToLocalChecked();
  }

  // window.chrome.braveSkus.refresh_order
  BindFunctionToObject(isolate, skus_obj, "refresh_order",
                       base::BindRepeating(&SkusPageController::RefreshOrder,
                                           base::Unretained(this), isolate));

  // window.chrome.braveSkus.fetch_order_credentials
  BindFunctionToObject(
      isolate, skus_obj, "fetch_order_credentials",
      base::BindRepeating(&SkusPageController::FetchOrderCredentials,
                          base::Unretained(this), isolate));

  // window.chrome.braveSkus.prepare_credentials_presentation
  BindFunctionToObject(
      isolate, skus_obj, "prepare_credentials_presentation",
      base::BindRepeating(&SkusPageController::PrepareCredentialsPresentation,
                          base::Unretained(this), isolate));

  // window.chrome.braveSkus.credential_summary
  BindFunctionToObject(
      isolate, skus_obj, "credential_summary",
      base::BindRepeating(&SkusPageController::CredentialSummary,
                          base::Unretained(this), isolate));
}

template <typename Sig>
void SkusPageController::BindFunctionToObject(
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

// window.chrome.braveSkus.refresh_order
v8::Local<v8::Promise> SkusPageController::RefreshOrder(v8::Isolate* isolate,
                                                        std::string order_id) {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();

  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }

  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  auto context_old(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));

  skus_service_->RefreshOrder(
      order_id,
      base::BindOnce(&SkusPageController::OnRefreshOrder,
                     base::Unretained(this), std::move(promise_resolver),
                     isolate, std::move(context_old)));

  return resolver.ToLocalChecked()->GetPromise();
}

void SkusPageController::OnRefreshOrder(
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Global<v8::Context> context_old,
    const std::string& response) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          response, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    v8::Local<v8::String> result =
        v8::String::NewFromUtf8(isolate, "Error parsing JSON response")
            .ToLocalChecked();
    ALLOW_UNUSED_LOCAL(resolver->Reject(context, result));
    return;
  }

  const base::DictionaryValue* result_dict;
  if (!records_v->GetAsDictionary(&result_dict)) {
    v8::Local<v8::String> result =
        v8::String::NewFromUtf8(isolate,
                                "Error converting response to dictionary")
            .ToLocalChecked();
    ALLOW_UNUSED_LOCAL(resolver->Reject(context, result));
    return;
  }

  v8::Local<v8::Value> local_result =
      content::V8ValueConverter::Create()->ToV8Value(result_dict, context);
  ALLOW_UNUSED_LOCAL(resolver->Resolve(context, local_result));
}

// window.chrome.braveSkus.fetch_order_credentials
v8::Local<v8::Promise> SkusPageController::FetchOrderCredentials(
    v8::Isolate* isolate,
    std::string order_id) {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();

  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }

  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  auto context_old(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));

  skus_service_->FetchOrderCredentials(
      order_id,
      base::BindOnce(&SkusPageController::OnFetchOrderCredentials,
                     base::Unretained(this), std::move(promise_resolver),
                     isolate, std::move(context_old)));

  return resolver.ToLocalChecked()->GetPromise();
}

void SkusPageController::OnFetchOrderCredentials(
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Global<v8::Context> context_old,
    const std::string& response) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  v8::Local<v8::String> result;
  result = v8::String::NewFromUtf8(isolate, response.c_str()).ToLocalChecked();

  ALLOW_UNUSED_LOCAL(resolver->Resolve(context, result));
}

// window.chrome.braveSkus.prepare_credentials_presentation
v8::Local<v8::Promise> SkusPageController::PrepareCredentialsPresentation(
    v8::Isolate* isolate,
    std::string domain,
    std::string path) {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();

  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }

  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  auto context_old(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));

  skus_service_->PrepareCredentialsPresentation(
      domain, path,
      base::BindOnce(&SkusPageController::OnPrepareCredentialsPresentation,
                     base::Unretained(this), std::move(promise_resolver),
                     isolate, std::move(context_old)));

  return resolver.ToLocalChecked()->GetPromise();
}

void SkusPageController::OnPrepareCredentialsPresentation(
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Global<v8::Context> context_old,
    const std::string& response) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  v8::Local<v8::String> result;
  result = v8::String::NewFromUtf8(isolate, response.c_str()).ToLocalChecked();

  ALLOW_UNUSED_LOCAL(resolver->Resolve(context, result));
}

// window.chrome.braveSkus.credential_summary
v8::Local<v8::Promise> SkusPageController::CredentialSummary(
    v8::Isolate* isolate,
    std::string domain) {
  if (!EnsureConnected())
    return v8::Local<v8::Promise>();

  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }

  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  auto context_old(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));

  skus_service_->CredentialSummary(
      domain,
      base::BindOnce(&SkusPageController::OnCredentialSummary,
                     base::Unretained(this), std::move(promise_resolver),
                     isolate, std::move(context_old)));

  return resolver.ToLocalChecked()->GetPromise();
}

void SkusPageController::OnCredentialSummary(
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Global<v8::Context> context_old,
    const std::string& response) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          response, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    v8::Local<v8::String> result =
        v8::String::NewFromUtf8(isolate, "Error parsing JSON response")
            .ToLocalChecked();
    ALLOW_UNUSED_LOCAL(resolver->Reject(context, result));
    return;
  }

  const base::DictionaryValue* result_dict;
  if (!records_v->GetAsDictionary(&result_dict)) {
    v8::Local<v8::String> result =
        v8::String::NewFromUtf8(isolate,
                                "Error converting response to dictionary")
            .ToLocalChecked();
    ALLOW_UNUSED_LOCAL(resolver->Reject(context, result));
    return;
  }

  v8::Local<v8::Value> local_result =
      content::V8ValueConverter::Create()->ToV8Value(result_dict, context);
  ALLOW_UNUSED_LOCAL(resolver->Resolve(context, local_result));
}

}  // namespace skus
