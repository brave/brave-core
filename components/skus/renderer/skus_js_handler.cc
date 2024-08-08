/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/renderer/skus_js_handler.h"

#include <optional>
#include <tuple>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#endif

namespace skus {

gin::WrapperInfo SkusJSHandler::kWrapperInfo = {gin::kEmbedderNativeGin};

SkusJSHandler::SkusJSHandler(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {}

SkusJSHandler::~SkusJSHandler() = default;

bool SkusJSHandler::EnsureConnected() {
  if (!skus_service_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker().GetInterface(
        skus_service_.BindNewPipeAndPassReceiver());
  }
  bool result = skus_service_.is_bound();
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (brave_vpn::IsBraveVPNFeatureEnabled()) {
    if (!vpn_service_.is_bound()) {
      render_frame()->GetBrowserInterfaceBroker().GetInterface(
          vpn_service_.BindNewPipeAndPassReceiver());
    }
    result = result && vpn_service_.is_bound();
  }
#endif

  return result;
}

void SkusJSHandler::Install(content::RenderFrame* render_frame) {
  CHECK(render_frame);
  v8::Isolate* isolate =
      render_frame->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame->GetWebFrame()->MainWorldScriptContext();
  if (context.IsEmpty())
    return;
  v8::Context::Scope context_scope(context);

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
  gin::Handle<SkusJSHandler> handler =
      gin::CreateHandle(isolate, new SkusJSHandler(render_frame));
  CHECK(!handler.IsEmpty());
  v8::PropertyDescriptor skus_desc(handler.ToV8(), false);
  skus_desc.set_configurable(false);

  chrome_obj
      ->DefineProperty(isolate->GetCurrentContext(),
                       gin::StringToV8(isolate, "braveSkus"), skus_desc)
      .Check();
}

void SkusJSHandler::OnDestruct() {
  delete this;
}

// window.chrome.braveSkus.refresh_order
v8::Local<v8::Promise> SkusJSHandler::RefreshOrder(v8::Isolate* isolate,
                                                   std::string order_id) {
  auto host = render_frame()->GetWebFrame()->GetSecurityOrigin().Host().Utf8();
  auto connected = EnsureConnected();
  if (!connected)
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
      host, order_id,
      base::BindOnce(&SkusJSHandler::OnRefreshOrder, base::Unretained(this),
                     std::move(promise_resolver), isolate,
                     std::move(context_old)));
  return resolver.ToLocalChecked()->GetPromise();
}

void SkusJSHandler::OnRefreshOrder(
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Global<v8::Context> context_old,
    skus::mojom::SkusResultPtr response) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);

  std::optional<base::Value> records_v = base::JSONReader::Read(
      response->message, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                             base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    v8::Local<v8::String> result =
        v8::String::NewFromUtf8(isolate, "Error parsing JSON response")
            .ToLocalChecked();
    std::ignore = resolver->Reject(context, result);
    return;
  }

  const base::Value::Dict* result_dict = records_v->GetIfDict();
  if (!result_dict) {
    v8::Local<v8::String> result =
        v8::String::NewFromUtf8(isolate,
                                "Error converting response to dictionary")
            .ToLocalChecked();
    std::ignore = resolver->Reject(context, result);
    return;
  }

  v8::Local<v8::Value> local_result =
      content::V8ValueConverter::Create()->ToV8Value(*result_dict, context);
  std::ignore = resolver->Resolve(context, local_result);
}

// window.chrome.braveSkus.fetch_order_credentials
v8::Local<v8::Promise> SkusJSHandler::FetchOrderCredentials(
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
  auto host = render_frame()->GetWebFrame()->GetSecurityOrigin().Host().Utf8();
  skus_service_->FetchOrderCredentials(
      host, order_id,
      base::BindOnce(&SkusJSHandler::OnFetchOrderCredentials,
                     base::Unretained(this), std::move(promise_resolver),
                     isolate, std::move(context_old)));

  return resolver.ToLocalChecked()->GetPromise();
}

void SkusJSHandler::OnFetchOrderCredentials(
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Global<v8::Context> context_old,
    skus::mojom::SkusResultPtr response) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  v8::Local<v8::String> result =
      v8::String::NewFromUtf8(isolate, response->message.c_str())
          .ToLocalChecked();

  if (response->message.empty()) {
    std::ignore = resolver->Resolve(context, result);
  } else {
    std::ignore = resolver->Reject(context, result);
  }
}

// window.chrome.braveSkus.prepare_credentials_presentation
v8::Local<v8::Promise> SkusJSHandler::PrepareCredentialsPresentation(
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
      base::BindOnce(&SkusJSHandler::OnPrepareCredentialsPresentation,
                     base::Unretained(this), std::move(promise_resolver),
                     isolate, std::move(context_old)));

  return resolver.ToLocalChecked()->GetPromise();
}

void SkusJSHandler::OnPrepareCredentialsPresentation(
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Global<v8::Context> context_old,
    skus::mojom::SkusResultPtr response) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  v8::Local<v8::String> result;
  result = v8::String::NewFromUtf8(isolate, response->message.c_str())
               .ToLocalChecked();

  std::ignore = resolver->Resolve(context, result);
}

// window.chrome.braveSkus.credential_summary
v8::Local<v8::Promise> SkusJSHandler::CredentialSummary(v8::Isolate* isolate,
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
      domain, base::BindOnce(&SkusJSHandler::OnCredentialSummary,
                             base::Unretained(this), domain,
                             std::move(promise_resolver), isolate,
                             std::move(context_old)));

  return resolver.ToLocalChecked()->GetPromise();
}

void SkusJSHandler::OnCredentialSummary(
    const std::string& domain,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    v8::Global<v8::Context> context_old,
    skus::mojom::SkusResultPtr response) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_old.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);

  std::optional<base::Value> records_v = base::JSONReader::Read(
      response->message, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                             base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    v8::Local<v8::String> result =
        v8::String::NewFromUtf8(isolate, "Error parsing JSON response")
            .ToLocalChecked();
    std::ignore = resolver->Reject(context, result);
    return;
  }

  const base::Value::Dict* result_dict = records_v->GetIfDict();
  if (!result_dict) {
    v8::Local<v8::String> result =
        v8::String::NewFromUtf8(isolate,
                                "Error converting response to dictionary")
            .ToLocalChecked();
    std::ignore = resolver->Reject(context, result);
    return;
  }
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (vpn_service_.is_bound()) {
    vpn_service_->LoadPurchasedState(domain);
  }
#endif
  v8::Local<v8::Value> local_result =
      content::V8ValueConverter::Create()->ToV8Value(*result_dict, context);
  std::ignore = resolver->Resolve(context, local_result);
}

gin::ObjectTemplateBuilder SkusJSHandler::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<SkusJSHandler>::GetObjectTemplateBuilder(isolate)
      .SetMethod("refresh_order", &SkusJSHandler::RefreshOrder)
      .SetMethod("fetch_order_credentials",
                 &SkusJSHandler::FetchOrderCredentials)
      .SetMethod("prepare_credentials_presentation",
                 &SkusJSHandler::PrepareCredentialsPresentation)
      .SetMethod("credential_summary", &SkusJSHandler::CredentialSummary);
}

}  // namespace skus
