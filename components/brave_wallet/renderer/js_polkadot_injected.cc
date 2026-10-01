/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_polkadot_injected.h"

#include <string_view>
#include <tuple>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "content/public/common/isolated_world_ids.h"
#include "gin/converter.h"
#include "gin/data_object_builder.h"
#include "gin/object_template_builder.h"
#include "v8/include/v8-container.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-json.h"
#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-object.h"
#include "v8/include/v8-primitive.h"
#include "v8/include/v8-promise.h"

namespace brave_wallet {

namespace {

constexpr char kNotImplementedError[] =
    "Signing is not implemented by this extension yet";
constexpr char kRequestFailedError[] = "Request failed.";
constexpr char kInvalidPayloadError[] = "Invalid payload.";

v8::Local<v8::Value> ToV8Error(v8::Isolate* isolate, std::string_view message) {
  return v8::Exception::Error(gin::StringToV8(isolate, message));
}

// `InjectedAccount`
v8::Local<v8::Value> ToV8Account(
    v8::Isolate* isolate,
    const mojom::PolkadotInjectedAccount& account) {
  return gin::DataObjectBuilder(isolate)
      .Set("address", account.address)
      .Set("genesisHash", v8::Null(isolate).As<v8::Value>())
      .Set("name", account.name)
      .Set("type", account.type)
      .Build();
}

v8::Local<v8::Promise> RejectedPromise(v8::Isolate* isolate,
                                       std::string_view message) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Promise::Resolver> resolver;
  if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) {
    return v8::Local<v8::Promise>();
  }
  std::ignore = resolver->Reject(context, ToV8Error(isolate, message));
  return resolver->GetPromise();
}

void Noop(const v8::FunctionCallbackInfo<v8::Value>&) {}

v8::MaybeLocal<v8::Function> NewNoopFunction(v8::Local<v8::Context> context) {
  return v8::Function::New(context, Noop, v8::Local<v8::Value>(),
                           /*length=*/0, v8::ConstructorBehavior::kThrow);
}

}  // namespace

JSPolkadotInjected::JSPolkadotInjected(
    mojo::Remote<mojom::PolkadotApi> remote,
    base::PassKey<class JSPolkadotProvider> pass_key,
    content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame), polkadot_api_(std::move(remote)) {}

JSPolkadotInjected::~JSPolkadotInjected() = default;

void JSPolkadotInjected::Cleanup() {
  polkadot_api_.reset();
  weak_ptr_factory_.InvalidateWeakPtrs();
  Dispose();
}

void JSPolkadotInjected::WillReleaseScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (world_id != content::ISOLATED_WORLD_ID_GLOBAL) {
    return;
  }
  Cleanup();
}

void JSPolkadotInjected::OnDestruct() {
  Cleanup();
}

v8::Local<v8::Value> JSPolkadotInjected::GetAccountsObject(
    v8::Isolate* isolate) {
  static constexpr ApiMethod kMethods[] = {
      {"get", &JSPolkadotInjected::AccountsGet},
      {"subscribe", &JSPolkadotInjected::AccountsSubscribe}};
  return BuildApiObject(isolate, kMethods);
}

v8::Local<v8::Value> JSPolkadotInjected::GetSignerObject(v8::Isolate* isolate) {
  static constexpr ApiMethod kMethods[] = {
      {"signPayload", &JSPolkadotInjected::SignerSignPayload},
      {"signRaw", &JSPolkadotInjected::SignerSignRaw}};
  return BuildApiObject(isolate, kMethods);
}

v8::Local<v8::Value> JSPolkadotInjected::BuildApiObject(
    v8::Isolate* isolate,
    base::span<const ApiMethod> methods) {
  // Use GetWrapper() to save ourselves as Data() for the `v8::Function`.
  v8::Local<v8::Object> self;
  if (!GetWrapper(isolate).ToLocal(&self)) {
    return v8::Local<v8::Value>();
  }

  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> api = v8::Object::New(isolate);
  for (const auto& method : methods) {
    v8::Local<v8::Function> function;
    if (!v8::Function::New(context, method.callback, self, /*length=*/1,
                           v8::ConstructorBehavior::kThrow)
             .ToLocal(&function)) {
      return v8::Local<v8::Value>();
    }

    if (CreateDataProperty(context, api, method.name, function).IsNothing()) {
      return v8::Local<v8::Value>();
    }
  }

  return api;
}

// static
JSPolkadotInjected* JSPolkadotInjected::InjectedFromCallbackData(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  JSPolkadotInjected* injected = nullptr;
  if (!gin::ConvertFromV8(info.GetIsolate(), info.Data(), &injected)) {
    return nullptr;
  }
  return injected;
}

// static
void JSPolkadotInjected::AccountsGet(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  JSPolkadotInjected* injected = InjectedFromCallbackData(info);
  if (!injected) {
    info.GetReturnValue().Set(RejectedPromise(isolate, kRequestFailedError));
    return;
  }

  info.GetReturnValue().Set(
      injected->GetAccounts(isolate, /*any_type=*/info[0]->IsTrue()));
}

// static
void JSPolkadotInjected::AccountsSubscribe(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  // We currently don't support subscription to accounts so there's nothing for
  // the return callback to unsubscribe from.
  v8::Local<v8::Function> unsubscribe;
  if (!NewNoopFunction(context).ToLocal(&unsubscribe)) {
    return;
  }

  JSPolkadotInjected* injected = InjectedFromCallbackData(info);
  if (injected && info[0]->IsFunction()) {
    v8::Local<v8::Promise> delivered;
    v8::Local<v8::Function> swallow_rejection;

    v8::Local<v8::Promise> accounts =
        injected->GetAccounts(isolate, /*any_type=*/false);

    if (!accounts.IsEmpty() &&
        accounts->Then(context, info[0].As<v8::Function>())
            .ToLocal(&delivered) &&
        NewNoopFunction(context).ToLocal(&swallow_rejection)) {
      std::ignore = delivered->Catch(context, swallow_rejection);
    }
  }

  info.GetReturnValue().Set(unsubscribe);
}

// static
void JSPolkadotInjected::SignerSignPayload(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  JSPolkadotInjected* injected = InjectedFromCallbackData(info);
  if (!injected) {
    info.GetReturnValue().Set(RejectedPromise(isolate, kRequestFailedError));
    return;
  }

  info.GetReturnValue().Set(injected->SignPayload(isolate, info[0]));
}

// static
void JSPolkadotInjected::SignerSignRaw(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  JSPolkadotInjected* injected = InjectedFromCallbackData(info);
  if (!injected) {
    info.GetReturnValue().Set(RejectedPromise(isolate, kRequestFailedError));
    return;
  }

  info.GetReturnValue().Set(injected->SignRaw(isolate, info[0]));
}

v8::Local<v8::Promise> JSPolkadotInjected::GetAccounts(v8::Isolate* isolate,
                                                       bool any_type) {
  if (!polkadot_api_.is_bound()) {
    return RejectedPromise(isolate, kRequestFailedError);
  }

  v8::Local<v8::Promise::Resolver> resolver_local;
  if (!v8::Promise::Resolver::New(isolate->GetCurrentContext())
           .ToLocal(&resolver_local)) {
    return v8::Local<v8::Promise>();
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver_local));

  polkadot_api_->GetAccounts(
      any_type,
      base::BindOnce(&JSPolkadotInjected::OnGetAccountsResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

void JSPolkadotInjected::OnGetAccountsResponse(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    std::optional<std::vector<mojom::PolkadotInjectedAccountPtr>> accounts,
    mojom::PolkadotProviderErrorBundlePtr error) {
  if (!render_frame()) {
    return;
  }
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);

  if (error || !accounts) {
    // No accounts should be considered an error considering the instantiation
    // of this object is predicated on the `enable()` flow running, with a user
    // manually authorizing with account they want the dApp to use.
    std::ignore = resolver->Reject(
        context,
        ToV8Error(isolate, error ? error->message : kRequestFailedError));
    return;
  }

  v8::LocalVector<v8::Value> values(isolate);
  values.reserve(accounts->size());
  for (const auto& account : *accounts) {
    values.push_back(ToV8Account(isolate, *account));
  }

  std::ignore = resolver->Resolve(
      context, v8::Array::New(isolate, values.data(), values.size()));
}

v8::Local<v8::Promise> JSPolkadotInjected::SignPayload(
    v8::Isolate* isolate,
    v8::Local<v8::Value> payload) {
  if (!polkadot_api_.is_bound()) {
    return RejectedPromise(isolate, kRequestFailedError);
  }

  if (!payload->IsObject()) {
    return RejectedPromise(isolate, kInvalidPayloadError);
  }

  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  // The payload is forwarded as JSON rather than field by field: the dApp's
  // `SignerPayloadJSON` is what the user is shown and what gets signed, so the
  // browser re-parses the same bytes we were handed.
  v8::Local<v8::String> payload_json;
  {
    // A getter or `toJSON` on the dApp's payload can throw.
    v8::TryCatch try_catch(isolate);
    if (!v8::JSON::Stringify(context, payload.As<v8::Object>())
             .ToLocal(&payload_json)) {
      return RejectedPromise(isolate, kInvalidPayloadError);
    }
  }

  v8::Local<v8::Promise::Resolver> resolver_local;
  if (!v8::Promise::Resolver::New(context).ToLocal(&resolver_local)) {
    return v8::Local<v8::Promise>();
  }

  auto global_context(v8::Global<v8::Context>(isolate, context));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver_local));

  polkadot_api_->SignPayload(
      gin::V8ToString(isolate, payload_json),
      base::BindOnce(&JSPolkadotInjected::OnSignPayloadResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

void JSPolkadotInjected::OnSignPayloadResponse(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    mojom::PolkadotSignerResultPtr result,
    mojom::PolkadotProviderErrorBundlePtr error) {
  if (!render_frame()) {
    return;
  }
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);

  if (error || !result) {
    std::ignore = resolver->Reject(
        context,
        ToV8Error(isolate, error ? error->message : kRequestFailedError));
    return;
  }

  v8::Local<v8::Value> signer_result = gin::DataObjectBuilder(isolate)
                                           .Set("id", result->id)
                                           .Set("signature", result->signature)
                                           .Build();
  std::ignore = resolver->Resolve(context, signer_result);
}

v8::Local<v8::Promise> JSPolkadotInjected::SignRaw(
    v8::Isolate* isolate,
    v8::Local<v8::Value> payload) {
  return RejectedPromise(isolate, kNotImplementedError);
}

// gin::Wrappable<JSPolkadotInjected>
gin::ObjectTemplateBuilder JSPolkadotInjected::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<JSPolkadotInjected>::GetObjectTemplateBuilder(isolate)
      .SetLazyDataProperty("accounts", &JSPolkadotInjected::GetAccountsObject)
      .SetLazyDataProperty("signer", &JSPolkadotInjected::GetSignerObject);
}

const gin::WrapperInfo* JSPolkadotInjected::wrapper_info() const {
  return &kWrapperInfo;
}

}  // namespace brave_wallet
