/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_polkadot_provider.h"

#include <string_view>
#include <tuple>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/types/pass_key.h"
#include "brave/components/brave_wallet/renderer/js_polkadot_injected.h"
#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "content/public/common/isolated_world_ids.h"
#include "gin/converter.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-object.h"
#include "v8/include/v8-primitive.h"
#include "v8/include/v8-promise.h"

namespace brave_wallet {

namespace {
constexpr char kInjectedWeb3[] = "injectedWeb3";
constexpr char kBraveWallet[] = "brave-wallet";
constexpr char kVersion[] = "1.0.0";
constexpr char kName[] = "Brave Wallet";
constexpr char kRequestFailedError[] = "Request failed";

// Assembles the nested `Injected` object around the flat JSPolkadotInjected
// wrapper. The closures keep the native object as the receiver of each call,
// which gin requires, and keep it alive for as long as the dapp holds the
// object. Called with (native, name, version, withExtensionInfo).
// https://github.com/polkadot-js/extension/blob/master/packages/extension-inject/src/types.ts
constexpr char kPolkadotInjectedScript[] = R"((function(
    native, name, version, withExtensionInfo) {
  const accounts = {
    get: (anyType) => native.getAccounts(anyType === true),
    // Single-shot: account changes aren't pushed to the page yet, so this
    // delivers the current list once and hands back a no-op unsubscribe. It is
    // what @polkadot/extension-dapp synthesizes for a wallet that omits
    // subscribe(), so dapps already handle it.
    subscribe: (cb) => {
      native.getAccounts(false).then(cb).catch(() => {});
      return () => {};
    }
  };
  const signer = {
    signPayload: (payload) => native.signPayload(payload),
    signRaw: (payload) => native.signRaw(payload)
  };
  const injected = { accounts, signer };
  if (withExtensionInfo) {
    injected.name = name;
    injected.version = version;
  }
  return injected;
}))";

}  // namespace

JSPolkadotProvider::JSPolkadotProvider(content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {
  EnsureConnected();
}

JSPolkadotProvider::~JSPolkadotProvider() = default;

void JSPolkadotProvider::WillReleaseScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (world_id != content::ISOLATED_WORLD_ID_GLOBAL) {
    return;
  }

  Cleanup();
}

void JSPolkadotProvider::OnDestruct() {
  Cleanup();
}

void JSPolkadotProvider::Cleanup() {
  polkadot_provider_.reset();
  weak_ptr_factory_.InvalidateWeakPtrs();
  Dispose();
}

bool JSPolkadotProvider::EnsureConnected() {
  if (!render_frame()) {
    return false;
  }

  if (!polkadot_provider_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker().GetInterface(
        polkadot_provider_.BindNewPipeAndPassReceiver());
  }

  return polkadot_provider_.is_bound();
}

std::string JSPolkadotProvider::GetVersion() {
  return kVersion;
}

v8::Local<v8::Promise> JSPolkadotProvider::Enable(gin::Arguments* args) {
  return RequestPermission(args->isolate(), /*with_extension_info=*/false);
}

v8::Local<v8::Promise> JSPolkadotProvider::Connect(gin::Arguments* args) {
  return RequestPermission(args->isolate(), /*with_extension_info=*/true);
}

v8::Local<v8::Promise> JSPolkadotProvider::RequestPermission(
    v8::Isolate* isolate,
    bool with_extension_info) {
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

  polkadot_provider_->Enable(base::BindOnce(
      &JSPolkadotProvider::OnEnableResponse, weak_ptr_factory_.GetWeakPtr(),
      std::move(global_context), std::move(promise_resolver), isolate,
      with_extension_info));

  return resolver_local->GetPromise();
}

void JSPolkadotProvider::OnEnableResponse(
    v8::Global<v8::Context> global_context,
    v8::Global<v8::Promise::Resolver> promise_resolver,
    v8::Isolate* isolate,
    bool with_extension_info,
    mojo::PendingRemote<mojom::PolkadotApi> pending_remote,
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

  auto reject = [&](std::string_view message) {
    std::ignore = resolver->Reject(
        context, v8::Exception::Error(gin::StringToV8(isolate, message)));
  };

  if (error) {
    reject(error->message);
    return;
  }

  if (!pending_remote) {
    reject(kRequestFailedError);
    return;
  }

  JSPolkadotInjected* injected = cppgc::MakeGarbageCollected<JSPolkadotInjected>(
      isolate->GetCppHeap()->GetAllocationHandle(),
      mojo::Remote<mojom::PolkadotApi>(std::move(pending_remote)),
      base::PassKey<JSPolkadotProvider>(), render_frame());

  v8::Local<v8::Object> injected_object;
  if (!injected->GetWrapper(isolate).ToLocal(&injected_object)) {
    reject(kRequestFailedError);
    return;
  }

  v8::Local<v8::Value> factory_value;
  if (!ExecuteScript(render_frame()->GetWebFrame(), kPolkadotInjectedScript)
           .ToLocal(&factory_value) ||
      !factory_value->IsFunction()) {
    reject(kRequestFailedError);
    return;
  }

  v8::LocalVector<v8::Value> args(
      isolate, {injected_object, gin::StringToV8(isolate, kName),
                gin::StringToV8(isolate, kVersion),
                v8::Boolean::New(isolate, with_extension_info)});

  v8::Local<v8::Value> result;
  if (!render_frame()
           ->GetWebFrame()
           ->CallFunctionEvenIfScriptDisabled(factory_value.As<v8::Function>(),
                                              context->Global(),
                                              static_cast<int>(args.size()),
                                              args.data())
           .ToLocal(&result)) {
    reject(kRequestFailedError);
    return;
  }

  std::ignore = resolver->Resolve(context, result);
}

// gin::Wrappable<JSPolkadotProvider>
gin::ObjectTemplateBuilder JSPolkadotProvider::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<JSPolkadotProvider>::GetObjectTemplateBuilder(isolate)
      .SetProperty("version", &JSPolkadotProvider::GetVersion)
      .SetMethod("enable", &JSPolkadotProvider::Enable)
      .SetMethod("connect", &JSPolkadotProvider::Connect);
}

const gin::WrapperInfo* JSPolkadotProvider::wrapper_info() const {
  return &kWrapperInfo;
}

// static
void JSPolkadotProvider::Install(content::RenderFrame* render_frame) {
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

  v8::Local<v8::Value> injected_web3;
  if (!global->Get(context, gin::StringToV8(isolate, kInjectedWeb3))
           .ToLocal(&injected_web3)) {
    return;
  }

  // Set window.injectedWeb3. Unlike the other provider roots this one stays
  // writable and configurable: it is a registry shared with every other
  // Polkadot wallet, and @polkadot/extension-inject reassigns the property
  // itself (`win.injectedWeb3 = win.injectedWeb3 || {}`) before adding its
  // own key, which would throw against a read-only property.
  if (!injected_web3->IsObject()) {
    injected_web3 = v8::Object::New(isolate);
    if (!global
             ->CreateDataProperty(context,
                                  gin::StringToV8(isolate, kInjectedWeb3),
                                  injected_web3)
             .FromMaybe(false)) {
      return;
    }
  }

  v8::Local<v8::Object> injected_web3_object;
  if (!injected_web3->ToObject(context).ToLocal(&injected_web3_object)) {
    return;
  }

  JSPolkadotProvider* polkadot_provider =
      cppgc::MakeGarbageCollected<JSPolkadotProvider>(
          isolate->GetCppHeap()->GetAllocationHandle(), render_frame);

  v8::Local<v8::Object> polkadot_provider_object =
      polkadot_provider->GetWrapper(isolate).ToLocalChecked();

  // Set window.injectedWeb3['brave-wallet']. Our own entry is read-only so
  // that another injecting wallet can't take it over.
  SetProviderNonWritable(
      context, injected_web3_object, polkadot_provider_object,
      gin::StringToV8(isolate, kBraveWallet), /*is_enumerable=*/true);

  // Non-function properties are readonly guaranteed by gin::Wrappable
  for (const std::string& method : {"enable", "connect"}) {
    SetOwnPropertyWritable(context, polkadot_provider_object,
                           gin::StringToV8(isolate, method), false);
  }
}

}  // namespace brave_wallet
