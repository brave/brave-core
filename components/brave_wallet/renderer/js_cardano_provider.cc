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

// gin::Wrappable<JSCardanoProvider>
gin::ObjectTemplateBuilder JSCardanoProvider::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<JSCardanoProvider>::GetObjectTemplateBuilder(isolate)
      .SetMethod("enable", &JSCardanoProvider::Enable);
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

  v8::MaybeLocal<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(isolate->GetCurrentContext());
  if (resolver.IsEmpty()) {
    return v8::Local<v8::Promise>();
  }

  auto global_context(
      v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));
  auto promise_resolver(
      v8::Global<v8::Promise::Resolver>(isolate, resolver.ToLocalChecked()));
  auto context(v8::Global<v8::Context>(isolate, isolate->GetCurrentContext()));

  cardano_provider_->Enable(base::BindOnce(
      &JSCardanoProvider::OnEnableResponse, weak_ptr_factory_.GetWeakPtr(),
      std::move(global_context), std::move(promise_resolver), isolate));

  return resolver.ToLocalChecked()->GetPromise();
}

void JSCardanoProvider::OnEnableResponse(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    const bool success) {
  if (!render_frame()) {
    return;
  }
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = global_context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Value> result = v8::Boolean::New(isolate, success);
  v8::Local<v8::Promise::Resolver> resolver = promise_resolver.Get(isolate);
  if (success) {
    gin::Handle<JSCardanoWalletApi> wallet_api = gin::CreateHandle(
        isolate, new JSCardanoWalletApi(base::PassKey<JSCardanoProvider>(),
                                        render_frame()));
    if (wallet_api.IsEmpty()) {
      return;
    }
    v8::Local<v8::Value> wallet_api_value = wallet_api.ToV8();
    v8::Local<v8::Object> wallet_api_object =
        wallet_api_value->ToObject(context).ToLocalChecked();
    std::ignore = resolver->Resolve(context, std::move(wallet_api_object));
  } else {
    std::ignore = resolver->Reject(context, result);
  }
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

  v8::Local<v8::Value> cardano_root =
      global->Get(context, gin::StringToV8(isolate, kCardano)).ToLocalChecked();

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
  v8::Local<v8::Object> cardano_brave_provider_object =
      cardano_brave_provider_value->ToObject(context).ToLocalChecked();

  // Set window.cardano.brave
  SetProviderNonWritable(
      context, cardano_root->ToObject(context).ToLocalChecked(),
      cardano_brave_provider_object, gin::StringToV8(isolate, kBrave), true);
}

}  // namespace brave_wallet
