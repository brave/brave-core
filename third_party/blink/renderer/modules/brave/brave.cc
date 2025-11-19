/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/brave/brave.h"

#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"

namespace blink {

Brave::Brave(NavigatorBase& navigator) : navigator_base_(navigator) {}

Brave* Brave::brave(NavigatorBase& navigator) {
  auto supplement = navigator.GetBraveNavigator();
  if (!supplement) {
    supplement = MakeGarbageCollected<Brave>(navigator);
    navigator.SetBraveNavigator(supplement);
  }
  return supplement;
}

ScriptPromise<IDLBoolean> Brave::isBrave(ScriptState* script_state) {
  auto* resolver =
      MakeGarbageCollected<ScriptPromiseResolver<IDLBoolean>>(script_state);
  ScriptPromise<IDLBoolean> promise = resolver->Promise();
  resolver->Resolve(true);
  return promise;
}

void Brave::Trace(Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
  visitor->Trace(navigator_base_);
}

}  // namespace blink
