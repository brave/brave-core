/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/brave/navigator_brave.h"

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "brave/third_party/blink/renderer/modules/brave/brave.h"
#include "brave/third_party/blink/renderer/modules/brave/brave_wallet.h"

namespace blink {

NavigatorBrave::NavigatorBrave(Navigator& navigator)
    : Supplement<Navigator>(navigator) {}

// static
const char NavigatorBrave::kSupplementName[] = "NavigatorBrave";

NavigatorBrave& NavigatorBrave::From(Navigator& navigator) {
  NavigatorBrave* supplement =
      Supplement<Navigator>::From<NavigatorBrave>(navigator);
  if (!supplement) {
    supplement = MakeGarbageCollected<NavigatorBrave>(navigator);
    ProvideTo(navigator, supplement);
  }
  return *supplement;
}

Brave* NavigatorBrave::brave(Navigator& navigator) {
  return NavigatorBrave::From(navigator).brave();
}

Brave* NavigatorBrave::brave() {
  if (!brave_) {
    brave_ = MakeGarbageCollected<Brave>();
  }
  return brave_;
}

BraveWallet* NavigatorBrave::brave_wallet(Navigator& navigator) {
  return NavigatorBrave::From(navigator).brave_wallet();
}

BraveWallet* NavigatorBrave::brave_wallet() {
  if (!brave_wallet_) {
    brave_wallet_ = MakeGarbageCollected<BraveWallet>();
  }
  return brave_wallet_;
}

void NavigatorBrave::Trace(blink::Visitor* visitor) const {
  visitor->Trace(brave_);
  visitor->Trace(brave_wallet_);
  Supplement<Navigator>::Trace(visitor);
}

}  // namespace blink
