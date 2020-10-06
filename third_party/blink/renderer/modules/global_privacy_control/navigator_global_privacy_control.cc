/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/global_privacy_control/navigator_global_privacy_control.h"

#include "third_party/blink/renderer/core/frame/navigator.h"

namespace blink {

NavigatorGlobalPrivacyControl::
  NavigatorGlobalPrivacyControl(Navigator& navigator) // NOLINT
    : Supplement<Navigator>(navigator) {}

// static
const char NavigatorGlobalPrivacyControl::kSupplementName[] =
    "NavigatorGlobalPrivacyControl";

NavigatorGlobalPrivacyControl&
  NavigatorGlobalPrivacyControl::From(Navigator& navigator) {
  NavigatorGlobalPrivacyControl* supplement =
      Supplement<Navigator>::From<NavigatorGlobalPrivacyControl>(navigator);
  if (!supplement) {
    supplement = MakeGarbageCollected<NavigatorGlobalPrivacyControl>(navigator);
    ProvideTo(navigator, supplement);
  }
  return *supplement;
}

bool NavigatorGlobalPrivacyControl::
  globalPrivacyControl(blink::Navigator& navigator) { // NOLINT
  return true;
}

void NavigatorGlobalPrivacyControl::Trace(blink::Visitor* visitor) const {
  Supplement<Navigator>::Trace(visitor);
}

}  // namespace blink
