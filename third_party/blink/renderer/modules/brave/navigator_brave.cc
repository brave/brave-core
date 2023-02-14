/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/brave/navigator_brave.h"

#include "brave/third_party/blink/renderer/modules/brave/brave.h"
#include "third_party/blink/renderer/core/execution_context/navigator_base.h"

namespace blink {

NavigatorBrave::NavigatorBrave(NavigatorBase& navigator)
    : Supplement<NavigatorBase>(navigator) {}

// static
const char NavigatorBrave::kSupplementName[] = "NavigatorBrave";

NavigatorBrave& NavigatorBrave::From(NavigatorBase& navigator) {
  NavigatorBrave* supplement =
      Supplement<NavigatorBase>::From<NavigatorBrave>(navigator);
  if (!supplement) {
    supplement = MakeGarbageCollected<NavigatorBrave>(navigator);
    ProvideTo(navigator, supplement);
  }
  return *supplement;
}

Brave* NavigatorBrave::brave(NavigatorBase& navigator) {
  return NavigatorBrave::From(navigator).brave();
}

Brave* NavigatorBrave::brave() {
  if (!brave_) {
    brave_ = MakeGarbageCollected<Brave>();
  }
  return brave_;
}

void NavigatorBrave::Trace(blink::Visitor* visitor) const {
  visitor->Trace(brave_);
  Supplement<NavigatorBase>::Trace(visitor);
}

}  // namespace blink
