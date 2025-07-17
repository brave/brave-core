/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_cardano_provider.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/components/brave_wallet/renderer/js_cardano_wallet_api.h"
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

namespace {
constexpr char kCardano[] = "cardano";
constexpr char kBrave[] = "brave";
}  // namespace

// content::RenderFrameObserver
void JSCardanoProvider::OnDestruct() {}

std::vector<std::string> JSCardanoProvider::GetSupportedExtensions() {
  return std::vector<std::string>();
}

std::string JSCardanoProvider::GetName() {
  return "Brave";
}

std::string JSCardanoProvider::GetIcon() {
  return "";
}

// gin::Wrappable<JSCardanoProvider>
gin::ObjectTemplateBuilder JSCardanoProvider::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<JSCardanoProvider>::GetObjectTemplateBuilder(isolate)
      .SetMethod("enable", &JSCardanoProvider::Enable)
      .SetMethod("isEnabled", &JSCardanoProvider::IsEnabled)
      .SetProperty("supportedExtensions",
                   &JSCardanoProvider::GetSupportedExtensions)
      .SetProperty("name", &JSCardanoProvider::GetName)
      .SetProperty("icon", &JSCardanoProvider::GetIcon);
}

const char* JSCardanoProvider::GetTypeName() {
  return "JSCardanoProvider";
}

// JSCardanoProvider
gin::WrapperInfo JSCardanoProvider::kWrapperInfo = {gin::kEmbedderNativeGin};

JSCardanoProvider::~JSCardanoProvider() = default;

JSCardanoProvider::JSCardanoProvider(content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {
  EnsureConnected();
}

bool JSCardanoProvider::EnsureConnected() {
  if (!render_frame()) {
    return false;
  }

  if (!cardano_provider_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker().GetInterface(
        cardano_provider_.BindNewPipeAndPassReceiver());
  }

  return cardano_provider_.is_bound();
}

v8::Local<v8::Promise> JSCardanoProvider::Enable(v8::Isolate* isolate) {
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

  cardano_provider_->Enable(base::BindOnce(
      &JSCardanoProvider::OnEnableResponse, weak_ptr_factory_.GetWeakPtr(),
      std::move(global_context), std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

void JSCardanoProvider::OnEnableResponse(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
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
    gin::Handle<JSCardanoWalletApi> wallet_api = gin::CreateHandle(
        isolate, new JSCardanoWalletApi(base::PassKey<JSCardanoProvider>(),
                                        global_context.Get(isolate), isolate,
                                        render_frame()));
    if (wallet_api.IsEmpty()) {
      return;
    }
    v8::Local<v8::Value> wallet_api_value = wallet_api.ToV8();
    v8::Local<v8::Object> wallet_api_object;
    if (!wallet_api_value->ToObject(context).ToLocal(&wallet_api_object)) {
      return;
    }

    // Non-function properties are readonly guaranteed by gin::Wrappable
    for (const std::string& method :
         {"getNetworkId", "getUsedAddresses", "getUnusedAddresses",
          "getChangeAddress", "getRewardAddresses", "getUtxos", "getBalance",
          "signTx", "signData", "submitTx", "getExtensions", "getCollateral"}) {
      SetOwnPropertyWritable(context, wallet_api_object,
                             gin::StringToV8(isolate, method), false);
    }

    std::ignore = resolver->Resolve(context, std::move(wallet_api_object));
  } else {
    std::ignore =
        resolver->Reject(context, ConvertError(isolate, context, error));
  }
}

v8::Local<v8::Promise> JSCardanoProvider::IsEnabled(v8::Isolate* isolate) {
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

  cardano_provider_->IsEnabled(base::BindOnce(
      &JSCardanoProvider::OnIsEnableResponse, weak_ptr_factory_.GetWeakPtr(),
      std::move(global_context), std::move(promise_resolver), isolate));

  return resolver_local->GetPromise();
}

void JSCardanoProvider::OnIsEnableResponse(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    const bool is_enabled) {
  if (!render_frame()) {
    return;
  }
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  std::ignore =
      resolver->Resolve(context, v8::Boolean::New(isolate, is_enabled));
}

// static
void JSCardanoProvider::Install(content::RenderFrame* render_frame) {
  // TODO(https://github.com/brave/brave-browser/issues/46369): Add proxy object
  // handler script
  CHECK(render_frame);
  v8::Isolate* isolate =
      render_frame->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame->GetWebFrame()->MainWorldScriptContext();
  if (context.IsEmpty()) {
    return;
  }

  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> global = context->Global();

  v8::Local<v8::Value> cardano_root;
  if (!global->Get(context, gin::StringToV8(isolate, kCardano))
           .ToLocal(&cardano_root)) {
    return;
  }

  if (cardano_root->IsUndefined() || !cardano_root->IsObject()) {
    cardano_root = v8::Object::New(isolate);
    // Set window.cardano
    SetProviderNonWritable(context, global, cardano_root,
                           gin::StringToV8(isolate, kCardano), true);
  }

  gin::Handle<JSCardanoProvider> cardano_brave_provider =
      gin::CreateHandle(isolate, new JSCardanoProvider(render_frame));
  if (cardano_brave_provider.IsEmpty()) {
    return;
  }
  v8::Local<v8::Value> cardano_brave_provider_value =
      cardano_brave_provider.ToV8();
  v8::Local<v8::Object> cardano_brave_provider_object;
  if (!cardano_brave_provider_value->ToObject(context).ToLocal(
          &cardano_brave_provider_object)) {
    return;
  }

  v8::Local<v8::Object> cardano_root_object;
  if (!cardano_root->ToObject(context).ToLocal(&cardano_root_object)) {
    return;
  }

  // Set window.cardano.brave
  SetProviderNonWritable(context, cardano_root_object,
                         cardano_brave_provider_object,
                         gin::StringToV8(isolate, kBrave), true);

  // Non-function properties are readonly guaranteed by gin::Wrappable
  for (const std::string& method : {"enable", "isEnabled"}) {
    SetOwnPropertyWritable(context, cardano_brave_provider_object,
                           gin::StringToV8(isolate, method), false);
  }
}

}  // namespace brave_wallet
