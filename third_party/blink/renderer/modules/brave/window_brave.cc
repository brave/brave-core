/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/brave/window_brave.h"

#include "brave/third_party/blink/renderer/modules/brave/ethereum.h"

namespace blink {

WindowBrave::WindowBrave(LocalDOMWindow& window)
    : Supplement<LocalDOMWindow>(window) {}

// static
const char WindowBrave::kSupplementName[] = "WindowBrave";

WindowBrave& WindowBrave::From(LocalDOMWindow& window) {
  WindowBrave* supplement =
      Supplement<LocalDOMWindow>::From<WindowBrave>(window);
  if (!supplement) {
    supplement = MakeGarbageCollected<WindowBrave>(window);
    ProvideTo(window, supplement);
  }
  return *supplement;
}

Ethereum* WindowBrave::ethereum(LocalDOMWindow& window) {
  return WindowBrave::From(window).ethereum();
}

Ethereum* WindowBrave::ethereum() {
  if (!ethereum_) {
    ethereum_ = MakeGarbageCollected<Ethereum>();
  }

  return ethereum_;
}

void WindowBrave::Trace(blink::Visitor* visitor) const {
  visitor->Trace(ethereum_);
  Supplement<LocalDOMWindow>::Trace(visitor);
}

}  // namespace blink
