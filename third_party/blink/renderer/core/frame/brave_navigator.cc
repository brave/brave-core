/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/frame/brave_navigator.h"
#include "brave/third_party/blink/renderer/core/frame/worker_brave.h"

namespace blink {

WorkerBrave* BraveNavigator::brave() {
  if (!brave_) {
    brave_ = MakeGarbageCollected<WorkerBrave>();
  }
  return brave_;
}

void BraveNavigator::Trace(blink::Visitor* visitor) const {
  visitor->Trace(brave_);
}

}  // namespace blink
