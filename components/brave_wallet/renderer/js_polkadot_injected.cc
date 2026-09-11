/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_polkadot_injected.h"

#include <string_view>
#include <tuple>
#include <utility>

#include "base/functional/bind.h"
#include "content/public/common/isolated_world_ids.h"
#include "gin/converter.h"
#include "gin/data_object_builder.h"
#include "gin/object_template_builder.h"
#include "v8/include/v8-container.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-primitive.h"
#include "v8/include/v8-promise.h"

namespace brave_wallet {

namespace {

constexpr char kNotImplementedError[] =
    "Signing is not implemented by this extension yet";

v8::Local<v8::Value> ToV8Error(v8::Isolate* isolate, std::string_view message) {
  return v8::Exception::Error(gin::StringToV8(isolate, message));
}

// `InjectedAccount`
v8::Local<v8::Value> ToV8Account(
    v8::Isolate* isolate,
    const mojom::PolkadotInjectedAccount& account) {
  return gin::DataObjectBuilder(isolate)
      .Set("address", account.address)
      .Set("genesisHash",
           account.genesis_hash
               ? gin::StringToV8(isolate, *account.genesis_hash).As<v8::Value>()
               : v8::Null(isolate).As<v8::Value>())
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

v8::Local<v8::Promise> JSPolkadotInjected::GetAccounts(v8::Isolate* isolate,
                                                       bool any_type) {
  if (!polkadot_api_.is_bound()) {
    return RejectedPromise(isolate, kNotImplementedError);
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
    std::ignore = resolver->Reject(
        context,
        ToV8Error(isolate, error ? error->message : kNotImplementedError));
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
  return RejectedPromise(isolate, kNotImplementedError);
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
      .SetMethod("getAccounts", &JSPolkadotInjected::GetAccounts)
      .SetMethod("signPayload", &JSPolkadotInjected::SignPayload)
      .SetMethod("signRaw", &JSPolkadotInjected::SignRaw);
}

const gin::WrapperInfo* JSPolkadotInjected::wrapper_info() const {
  return &kWrapperInfo;
}

}  // namespace brave_wallet
