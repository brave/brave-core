/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/brave/skus/skus.h"

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"

namespace blink {

/*OrderCallback() {
  // does rust need to execute in browser process?

  // needs to get to browser process
  browser_process->GetCurrentProfile()
}*/

// ensure order is paid
ScriptPromise Skus::refresh_order(ScriptState* script_state,
                                  uint32_t order_id) {
  auto* resolver = MakeGarbageCollected<ScriptPromiseResolver>(script_state);
  ScriptPromise promise = resolver->Promise();

  // TODO (bsclifton): set callback; call rust SDK here
  resolver->Resolve(true);

  return promise;
}

// retrieves the credentials and stores them into profile storage
ScriptPromise Skus::fetch_order_credentials(ScriptState* script_state,
                                            uint32_t order_id) {
  auto* resolver = MakeGarbageCollected<ScriptPromiseResolver>(script_state);
  ScriptPromise promise = resolver->Promise();

  // TODO (bsclifton): set callback; call rust SDK here
  resolver->Resolve(true);

  return promise;
}

}  // namespace blink
