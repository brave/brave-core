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
#include "v8/include/v8-isolate.h"
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

// The `native` object here implements the primary interface, which is our
// JSPolkadotInjected object.
//
// For the definition of Injected:
// https://github.com/polkadot-js/extension/blob/d7c9ce214557e8bd359fac29c4bc38d0e329c1d4/packages/extension-inject/src/types.ts#L96-L101
constexpr char kPolkadotInjectedScript[] = R"((function(
    native, name, version, withExtensionInfo) {
  const accounts = {
    get: (anyType) => native.getAccounts(anyType === true),
    // Currently implement subscribe as a no-op.
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

void InjectedWeb3Getter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(info.Data());
}

// Throw if any script attempts to replace the `injectedWeb3` object itself,
// aside from self-assignment via `injectedWeb3 = injectedWeb3 || {}`.
// This is so no script will be able to swap out the wallet registry.
void InjectedWeb3Setter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info[0]->StrictEquals(info.Data())) {
    return;
  }

  v8::Isolate* isolate = info.GetIsolate();
  isolate->ThrowException(v8::Exception::TypeError(
      gin::StringToV8(isolate, "window.injectedWeb3 cannot be replaced.")));
}

}  // namespace

JSPolkadotProvider::JSPolkadotProvider(content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {
  EnsureConnected();
}

JSPolkadotProvider::~JSPolkadotProvider() = default;

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

  // The registry is shared with every other Polkadot wallet, so append
  // ourselves if the registry exists otherwise create it now.
  v8::Local<v8::Object> injected_web3_object =
      injected_web3->IsObject() ? injected_web3.As<v8::Object>()
                                : v8::Object::New(isolate);

  v8::Local<v8::Function> getter;
  v8::Local<v8::Function> setter;
  if (!v8::Function::New(context, InjectedWeb3Getter, injected_web3_object,
                         /*length=*/0, v8::ConstructorBehavior::kThrow,
                         v8::SideEffectType::kHasNoSideEffect)
           .ToLocal(&getter) ||
      !v8::Function::New(context, InjectedWeb3Setter, injected_web3_object,
                         /*length=*/1, v8::ConstructorBehavior::kThrow)
           .ToLocal(&setter)) {
    return;
  }

  v8::PropertyDescriptor injected_web3_desc(getter, setter);
  injected_web3_desc.set_enumerable(true);
  injected_web3_desc.set_configurable(false);

  {
    // Update the injectedWeb3 registry to be non-assignable, throwing if a
    // malicious script attempts `window.injectedWeb3 = {...}` instead of
    // `window.injectedWeb3 = window.injectedWeb3 || {}`, which is permitted.
    // Another wallet may've already done something similar to what we're
    // attempting, so DefineProperty is a best-effort attempt and can't be
    // guaranteed.
    v8::TryCatch try_catch(isolate);
    std::ignore = global->DefineProperty(
        context, gin::StringToV8(isolate, kInjectedWeb3), injected_web3_desc);
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

bool JSPolkadotProvider::EnsureConnected() {
  if (!render_frame()) {
    return false;
  }

  if (!polkadot_provider_.is_bound()) {
    // Trigger our pre-registered callback invoking
    // BraveWalletTabHelper::BindPolkadotProvider by the
    // BraveContentBrowserClient.
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

  JSPolkadotInjected* injected =
      cppgc::MakeGarbageCollected<JSPolkadotInjected>(
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
           ->CallFunctionEvenIfScriptDisabled(
               factory_value.As<v8::Function>(), context->Global(),
               static_cast<int>(args.size()), args.data())
           .ToLocal(&result)) {
    reject(kRequestFailedError);
    return;
  }

  std::ignore = resolver->Resolve(context, result);
}

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

}  // namespace brave_wallet
