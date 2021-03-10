/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/brave/brave_wallet.h"

#include <string>

#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_throw_dom_exception.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"

namespace blink {

bool BraveWallet::EnsureConnected(ExecutionContext* execution_context) {
  if (!brave_wallet_provider_.is_bound()) {
    execution_context->GetBrowserInterfaceBroker().GetInterface(
        brave_wallet_provider_.BindNewPipeAndPassReceiver());
  }

  return brave_wallet_provider_.is_bound();
}

ScriptPromise BraveWallet::request(ScriptState* script_state,
                                   const String& input) {
  if (!EnsureConnected(ExecutionContext::From(script_state)))
    return ScriptPromise();

  auto* resolver = MakeGarbageCollected<ScriptPromiseResolver>(script_state);
  ScriptPromise promise = resolver->Promise();

  brave_wallet_provider_->Request(input.Utf8(), WTF::Bind(
      [](ScriptPromiseResolver* resolver, BraveWallet* brave_wallet,
          const int status, const std::string& response) {
        DCHECK(resolver);

        if (response.empty()) {
          ScriptState* state = resolver->GetScriptState();
          ScriptState::Scope scope(state);

          std::ostringstream os;
          os << "brave_wallet error: status = " << status <<
              ", response = " << response;

          resolver->Reject(V8ThrowDOMException::CreateOrEmpty(
              state->GetIsolate(), DOMExceptionCode::kDataError,
              blink::WebString::FromUTF8(os.str())));
        } else {
          resolver->Resolve(blink::WebString::FromUTF8(response));
        }
      },
      WrapPersistent(resolver), WrapPersistent(this)));

  return promise;
}

}  // namespace blink
