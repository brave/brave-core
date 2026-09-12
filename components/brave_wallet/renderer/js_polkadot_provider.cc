/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/js_polkadot_provider.h"

#include "base/check.h"
#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "content/public/common/isolated_world_ids.h"
#include "gin/converter.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-object.h"

namespace brave_wallet {

namespace {
constexpr char kInjectedWeb3[] = "injectedWeb3";
constexpr char kBraveWallet[] = "brave-wallet";
constexpr char kVersion[] = "1.0.0";

}  // namespace

JSPolkadotProvider::JSPolkadotProvider(content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {}

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
  Dispose();
}

std::string JSPolkadotProvider::GetVersion() {
  return kVersion;
}

// gin::Wrappable<JSPolkadotProvider>
gin::ObjectTemplateBuilder JSPolkadotProvider::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<JSPolkadotProvider>::GetObjectTemplateBuilder(isolate)
      .SetProperty("version", &JSPolkadotProvider::GetVersion);
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
}

}  // namespace brave_wallet
