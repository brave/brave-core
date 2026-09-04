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
// Version of the object we inject, not of the browser: dApps see it verbatim,
// so it must not carry build-level fingerprinting entropy.
constexpr char kVersion[] = "1.0.0";

// DEVELOPMENT ONLY. Stands up a fake `enable()` so dApps complete discovery,
// and logs everything they hand us on the way through. The accounts are
// synthetic and the signatures are garbage; this must not ship.
constexpr char kPolkadotDevShimScript[] = R"((function() {
  console.log('do I even see this????');

  const TAG = '%c[brave-polkadot]';
  const STYLE = 'color:#f0f;font-weight:bold';
  const log = (what, ...rest) => console.log(TAG + ' ' + what, STYLE, ...rest);

  // Synthetic accounts. genesisHash must be null: @polkadot/extension-dapp
  // filters accounts whose genesisHash doesn't match the chain the dApp is on,
  // so a real hash here makes accounts silently vanish on every other chain.
  const ACCOUNTS = [
    {
      address: '5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY',
      name: 'Alice (synthetic)',
      type: 'sr25519',
      genesisHash: null,
    },
    {
      address: '5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty',
      name: 'Bob (synthetic)',
      type: 'sr25519',
      genesisHash: null,
    },
  ];

  // Returning a well-formed dummy lets the dApp keep going and build/submit the
  // extrinsic, which is where the interesting logging is. The node rejects it
  // with BadProof. Flip to false to hard-stop at the signing call instead.
  const RESOLVE_SIGNATURES = true;
  const DUMMY_SIGNATURE = '0x01' + 'ab'.repeat(64);

  let signId = 0;
  const finish = (label) => {
    if (!RESOLVE_SIGNATURES) {
      throw new Error('brave-wallet: ' + label + ' not implemented');
    }
    return { id: ++signId, signature: DUMMY_SIGNATURE };
  };

  window.injectedWeb3['talisman'] = {};
  const provider = window.injectedWeb3['talisman'];

  provider.enable = async (originName) => {
    log('enable(originName)', originName, '| page:', location.href);

    return {
      accounts: {
        get: async (anyType) => {
          log('accounts.get(anyType)', anyType);
          return ACCOUNTS;
        },
        subscribe: (cb) => {
          log('accounts.subscribe(cb)');
          Promise.resolve().then(() => cb(ACCOUNTS));
          return () => log('accounts.unsubscribe()');
        },
      },
      // dApps push chain metadata through here; worth seeing what they think
      // we should know about the chain.
      metadata: {
        get: async () => {
          log('metadata.get()');
          return [];
        },
        provide: async (def) => {
          log('metadata.provide(def)', def);
          return true;
        },
      },
      signer: {
        signPayload: async (payload) => {
          log('signer.signPayload(payload)', payload);
          log('  payload JSON\n' + JSON.stringify(payload, null, 2));
          return finish('signPayload');
        },
        signRaw: async (raw) => {
          log('signer.signRaw(raw)', raw);
          log('  raw JSON\n' + JSON.stringify(raw, null, 2));
          return finish('signRaw');
        },
        update: (id, status) => log('signer.update(id, status)', id, status),
      },
    };
  };
})())";
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
  // No longer need that provider object. Clean bound v8 references, stop
  // tracking the render frame.
  Dispose();
}

void JSPolkadotProvider::OnDestruct() {
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

  if (!injected_web3->IsObject()) {
    injected_web3 = v8::Object::New(isolate);
    // Set window.injectedWeb3. Unlike the other provider roots this one stays
    // writable and configurable: it is a registry shared with every other
    // Polkadot wallet, and @polkadot/extension-inject reassigns the property
    // itself (`win.injectedWeb3 = win.injectedWeb3 || {}`) before adding its
    // own key, which would throw against a read-only property.
    if (!CreateDataProperty(context, global, kInjectedWeb3, injected_web3)
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
  SetProviderNonWritable(context, injected_web3_object,
                         polkadot_provider_object,
                         gin::StringToV8(isolate, kBraveWallet), true);

  // Locks the registry slot, not the object, so the shim can still hang
  // `enable` off the wrapper.
  ExecuteScript(render_frame->GetWebFrame(), kPolkadotDevShimScript);
}

}  // namespace brave_wallet
