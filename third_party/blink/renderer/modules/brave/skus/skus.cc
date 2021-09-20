/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/brave/skus/skus.h"

#include "content/public/browser/service_process_host.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"

namespace blink {

Skus::~Skus() {
  skus_sdk_caller_.reset();
}

// ensure order is paid
ScriptPromise Skus::refresh_order(ScriptState* script_state,
                                  uint32_t order_id) {
  auto* resolver = MakeGarbageCollected<ScriptPromiseResolver>(script_state);
  ScriptPromise promise = resolver->Promise();

///////////////////////
  content::ServiceProcessHost::Launch(
      skus_sdk_caller_.BindNewPipeAndPassReceiver(),
      content::ServiceProcessHost::Options()
          .WithDisplayName("LOL Utility Process")
          .Pass());

  //TODO: need to resolve linking

  // skus_sdk_caller_.set_disconnect_handler(
  //     base::BindOnce(&ExternalProcessImporterClient::OnProcessCrashed, this));

  skus_sdk_caller_->StartRefreshOrder(order_id);

  ///////////////////////

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

void Skus::Trace(blink::Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
