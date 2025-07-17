/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_cardano_wallet_api.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "content/public/common/isolated_world_ids.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/converter.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-proxy.h"
#include "v8/include/v8-typed-array.h"

namespace brave_wallet {

// See Error Types section from https://cips.cardano.org/cip/CIP-30
v8::Local<v8::Value> ConvertError(
    v8::Isolate* isolate,
    const v8::Local<v8::Context>& context,
    const mojom::CardanoProviderErrorBundlePtr& error) {
  CHECK(error);
  base::Value::Dict error_value;

  if (error->pagination_error_payload) {
    error_value.Set("maxNumber",
                    base::Value(error->pagination_error_payload->payload));
    return content::V8ValueConverter::Create()->ToV8Value(error_value, context);
  }

  error_value.Set("code", base::Value(error->code));
  error_value.Set("info", error->error_message);

  return content::V8ValueConverter::Create()->ToV8Value(error_value, context);
}

// content::RenderFrameObserver
void JSCardanoWalletApi::OnDestruct() {}

// gin::Wrappable<JSCardanoProvider>
gin::ObjectTemplateBuilder JSCardanoWalletApi::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<JSCardanoWalletApi>::GetObjectTemplateBuilder(isolate)
      .SetMethod("getNetworkId", &JSCardanoWalletApi::GetNetworkId)
      .SetMethod("getUsedAddresses", &JSCardanoWalletApi::GetUsedAddresses)
      .SetMethod("getUnusedAddresses", &JSCardanoWalletApi::GetUnusedAddresses)
      .SetMethod("getChangeAddress", &JSCardanoWalletApi::GetChangeAddress)
      .SetMethod("getRewardAddresses", &JSCardanoWalletApi::GetRewardAddresses)
      .SetMethod("getUtxos", &JSCardanoWalletApi::GetUtxos)
      .SetMethod("getBalance", &JSCardanoWalletApi::GetBalance)
      .SetMethod("signTx", &JSCardanoWalletApi::SignTx)
      .SetMethod("signData", &JSCardanoWalletApi::SignData)
      .SetMethod("submitTx", &JSCardanoWalletApi::SubmitTx)
      .SetMethod("getExtensions", &JSCardanoWalletApi::GetExtensions)
      .SetMethod("getCollateral", &JSCardanoWalletApi::GetCollateral);
}

const char* JSCardanoWalletApi::GetTypeName() {
  return "JSCardanoWalletApi";
}

// JSCardanoProvider
gin::WrapperInfo JSCardanoWalletApi::kWrapperInfo = {gin::kEmbedderNativeGin};

JSCardanoWalletApi::~JSCardanoWalletApi() = default;

JSCardanoWalletApi::JSCardanoWalletApi(
    base::PassKey<class JSCardanoProvider> pass_key,
    v8::Local<v8::Context> context,
    v8::Isolate* isolate,
    content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {
  EnsureConnected();
}

bool JSCardanoWalletApi::EnsureConnected() {
  if (!render_frame()) {
    return false;
  }

  if (!cardano_provider_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker().GetInterface(
        cardano_provider_.BindNewPipeAndPassReceiver());
  }

  return cardano_provider_.is_bound();
}

void JSCardanoWalletApi::HandleStringResult(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    const std::string& result,
    mojom::CardanoProviderErrorBundlePtr error) {
  if (!render_frame()) {
    return;
  }
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  if (!error) {
    std::ignore = resolver->Resolve(
        context, gin::StringToV8(context->GetIsolate(), result));
  } else {
    std::ignore = resolver->Reject(
        context, ConvertError(context->GetIsolate(), context, error));
  }
}

void JSCardanoWalletApi::HandleStringVecResult(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    const std::vector<std::string>& result,
    mojom::CardanoProviderErrorBundlePtr error) {
  if (!render_frame()) {
    return;
  }
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  base::ListValue list_value;
  for (const auto& item : result) {
    list_value.Append(base::Value(item));
  }

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  if (!error) {
    std::ignore = resolver->Resolve(
        context,
        content::V8ValueConverter::Create()->ToV8Value(list_value, context));
  } else {
    std::ignore = resolver->Reject(
        context, ConvertError(context->GetIsolate(), context, error));
  }
}

void JSCardanoWalletApi::HandleUtxoVecResult(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    const std::optional<std::vector<std::string>>& result,
    mojom::CardanoProviderErrorBundlePtr error) {
  if (!render_frame()) {
    return;
  }
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  if (!error) {
    base::ListValue list_value;
    for (const auto& item : result.value()) {
      list_value.Append(base::Value(item));
    }

    std::ignore = resolver->Resolve(
        context,
        content::V8ValueConverter::Create()->ToV8Value(list_value, context));
  } else {
    std::ignore = resolver->Reject(
        context, ConvertError(context->GetIsolate(), context, error));
  }
}

v8::Local<v8::Promise> JSCardanoWalletApi::GetNetworkId(v8::Isolate* isolate) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
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
  auto context(v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));

  cardano_provider_->GetNetworkId(base::BindOnce(
      &JSCardanoWalletApi::OnGetNetworkId, weak_ptr_factory_.GetWeakPtr(),
      std::move(global_context), std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

void JSCardanoWalletApi::OnGetNetworkId(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    int32_t network,
    mojom::CardanoProviderErrorBundlePtr error) {
  if (!render_frame()) {
    return;
  }
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  if (!error) {
    std::ignore = resolver->Resolve(context, v8::Int32::New(isolate, network));
  } else {
    std::ignore = resolver->Reject(
        context, ConvertError(context->GetIsolate(), context, error));
  }
}

v8::Local<v8::Promise> JSCardanoWalletApi::GetUsedAddresses(
    v8::Isolate* isolate) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
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

  cardano_provider_->GetUsedAddresses(
      base::BindOnce(&JSCardanoWalletApi::HandleStringVecResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

v8::Local<v8::Promise> JSCardanoWalletApi::GetUnusedAddresses(
    v8::Isolate* isolate) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
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

  cardano_provider_->GetUnusedAddresses(
      base::BindOnce(&JSCardanoWalletApi::HandleStringVecResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

v8::Local<v8::Promise> JSCardanoWalletApi::GetChangeAddress(
    v8::Isolate* isolate) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
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

  cardano_provider_->GetChangeAddress(base::BindOnce(
      &JSCardanoWalletApi::HandleStringResult, weak_ptr_factory_.GetWeakPtr(),
      std::move(global_context), std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

v8::Local<v8::Promise> JSCardanoWalletApi::GetRewardAddresses(
    v8::Isolate* isolate) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
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

  cardano_provider_->GetRewardAddresses(
      base::BindOnce(&JSCardanoWalletApi::HandleStringVecResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

v8::Local<v8::Promise> JSCardanoWalletApi::GetBalance(v8::Isolate* isolate) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
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

  cardano_provider_->GetBalance(base::BindOnce(
      &JSCardanoWalletApi::HandleStringResult, weak_ptr_factory_.GetWeakPtr(),
      std::move(global_context), std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

v8::Local<v8::Promise> JSCardanoWalletApi::GetUtxos(gin::Arguments* args) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
  }
  auto* isolate = args->isolate();
  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));

  v8::Local<v8::Promise::Resolver> resolver_local;
  if (!v8::Promise::Resolver::New(isolate->GetCurrentContext())
           .ToLocal(&resolver_local)) {
    return v8::Local<v8::Promise>();
  }

  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver_local));

  auto arguments = args->GetAll();

  std::optional<std::string> amount;
  mojom::CardanoProviderPaginationPtr page;

  if (arguments.size() > 2) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  if (arguments.size() > 0) {
    std::unique_ptr<base::Value> arg1_value =
        content::V8ValueConverter::Create()->FromV8Value(
            arguments.at(0), isolate->GetCurrentContext());
    if (!arg1_value || !arg1_value->GetIfString()) {
      args->ThrowError();
      return v8::Local<v8::Promise>();
    }
    amount = arg1_value->GetString();
  }

  if (arguments.size() > 1) {
    std::unique_ptr<base::Value> arg2_value =
        content::V8ValueConverter::Create()->FromV8Value(
            arguments.at(1), isolate->GetCurrentContext());
    if (!arg2_value || !arg2_value->GetIfDict()) {
      args->ThrowError();
      return v8::Local<v8::Promise>();
    }

    auto page_value = arg2_value->GetIfDict()->FindInt("page");
    auto page_limit = arg2_value->GetIfDict()->FindInt("limit");

    if (!page_value || !page_limit) {
      args->ThrowError();
      return v8::Local<v8::Promise>();
    }

    page = mojom::CardanoProviderPagination::New(*page_value, *page_limit);
  }

  cardano_provider_->GetUtxos(
      amount, std::move(page),
      base::BindOnce(&JSCardanoWalletApi::HandleUtxoVecResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

v8::Local<v8::Promise> JSCardanoWalletApi::SignTx(gin::Arguments* args) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
  }

  auto* isolate = args->isolate();

  v8::Local<v8::Promise::Resolver> resolver_local;
  if (!v8::Promise::Resolver::New(isolate->GetCurrentContext())
           .ToLocal(&resolver_local)) {
    return v8::Local<v8::Promise>();
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver_local));

  auto arguments = args->GetAll();

  if (arguments.size() != 1 && arguments.size() != 2) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  std::unique_ptr<base::Value> arg1_value =
      content::V8ValueConverter::Create()->FromV8Value(
          arguments.at(0), isolate->GetCurrentContext());

  if (!arg1_value || !arg1_value->GetIfString()) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  bool partial_sign = false;
  if (arguments.size() == 2) {
    std::unique_ptr<base::Value> arg2_value =
        content::V8ValueConverter::Create()->FromV8Value(
            arguments.at(1), isolate->GetCurrentContext());

    if (!arg2_value || !arg2_value->GetIfBool()) {
      args->ThrowError();
      return v8::Local<v8::Promise>();
    }

    partial_sign = arg2_value->GetIfBool().value_or(false);
  }

  cardano_provider_->SignTx(
      arg1_value->GetString(), partial_sign,
      base::BindOnce(&JSCardanoWalletApi::HandleStringResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

v8::Local<v8::Promise> JSCardanoWalletApi::SignData(gin::Arguments* args) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
  }

  auto* isolate = args->isolate();

  v8::Local<v8::Promise::Resolver> resolver_local;
  if (!v8::Promise::Resolver::New(isolate->GetCurrentContext())
           .ToLocal(&resolver_local)) {
    return v8::Local<v8::Promise>();
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver_local));

  auto arguments = args->GetAll();

  if (arguments.size() != 2) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  std::unique_ptr<base::Value> arg1_value =
      content::V8ValueConverter::Create()->FromV8Value(
          arguments.at(0), isolate->GetCurrentContext());
  std::unique_ptr<base::Value> arg2_value =
      content::V8ValueConverter::Create()->FromV8Value(
          arguments.at(1), isolate->GetCurrentContext());

  if (!arg1_value || !arg2_value || !arg1_value->GetIfString() ||
      !arg2_value->GetIfString()) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  cardano_provider_->SignData(
      arg1_value->GetString(), arg2_value->GetString(),
      base::BindOnce(&JSCardanoWalletApi::OnSignData,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

void JSCardanoWalletApi::OnSignData(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    mojom::CardanoProviderSignatureResultPtr result,
    mojom::CardanoProviderErrorBundlePtr error) {
  if (!render_frame()) {
    return;
  }
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  if (!error) {
    base::Value::Dict dict;
    dict.Set("signature", base::Value(result->signature));
    dict.Set("key", base::Value(result->key));
    std::ignore = resolver->Resolve(
        context, content::V8ValueConverter::Create()->ToV8Value(
                     dict, isolate->GetCurrentContext()));
  } else {
    std::ignore = resolver->Reject(
        context, ConvertError(context->GetIsolate(), context, error));
  }
}

v8::Local<v8::Promise> JSCardanoWalletApi::SubmitTx(gin::Arguments* args) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
  }

  auto* isolate = args->isolate();

  v8::Local<v8::Promise::Resolver> resolver_local;
  if (!v8::Promise::Resolver::New(isolate->GetCurrentContext())
           .ToLocal(&resolver_local)) {
    return v8::Local<v8::Promise>();
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver_local));

  auto arguments = args->GetAll();

  if (arguments.size() != 1) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  std::unique_ptr<base::Value> arg1_value =
      content::V8ValueConverter::Create()->FromV8Value(
          arguments.at(0), isolate->GetCurrentContext());
  if (!arg1_value || !arg1_value->GetIfString()) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  cardano_provider_->SubmitTx(
      arg1_value->GetString(),
      base::BindOnce(&JSCardanoWalletApi::HandleStringResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

v8::Local<v8::Promise> JSCardanoWalletApi::GetExtensions(gin::Arguments* args) {
  auto* isolate = args->isolate();

  v8::Local<v8::Promise::Resolver> resolver_local;
  if (!v8::Promise::Resolver::New(isolate->GetCurrentContext())
           .ToLocal(&resolver_local)) {
    return v8::Local<v8::Promise>();
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));

  std::ignore = resolver_local->Resolve(
      global_context.Get(isolate),
      content::V8ValueConverter::Create()->ToV8Value(
          base::ListValue(), isolate->GetCurrentContext()));

  return resolver_local->GetPromise();
}

v8::Local<v8::Promise> JSCardanoWalletApi::GetCollateral(gin::Arguments* args) {
  if (!EnsureConnected()) {
    return v8::Local<v8::Promise>();
  }

  auto* isolate = args->isolate();
  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  v8::Local<v8::Promise::Resolver> resolver_local;
  if (!v8::Promise::Resolver::New(isolate->GetCurrentContext())
           .ToLocal(&resolver_local)) {
    return v8::Local<v8::Promise>();
  }
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver_local));

  auto arguments = args->GetAll();

  if (arguments.size() != 1) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  std::unique_ptr<base::Value> arg1_value =
      content::V8ValueConverter::Create()->FromV8Value(
          arguments.at(0), isolate->GetCurrentContext());

  if (!arg1_value || !arg1_value->GetIfDict()) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  std::string* amount_property = arg1_value->GetDict().FindString("amount");
  if (!amount_property) {
    args->ThrowError();
    return v8::Local<v8::Promise>();
  }

  cardano_provider_->GetCollateral(
      *amount_property,
      base::BindOnce(&JSCardanoWalletApi::HandleUtxoVecResult,
                     weak_ptr_factory_.GetWeakPtr(), std::move(global_context),
                     std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

}  // namespace brave_wallet
