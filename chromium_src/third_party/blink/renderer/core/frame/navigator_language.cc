/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/navigator_language.h"

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"

#define NavigatorLanguage NavigatorLanguage_ChromiumImpl
#include "src/third_party/blink/renderer/core/frame/navigator_language.cc"
#undef NavigatorLanguage

namespace blink {

NavigatorLanguage::NavigatorLanguage(ExecutionContext* execution_context)
    : NavigatorLanguage_ChromiumImpl(execution_context) {}

void NavigatorLanguage::EnsureUpdatedLanguage() {
  NavigatorLanguage_ChromiumImpl::EnsureUpdatedLanguage();
  BraveFarblingLevel farbling_level = brave::GetBraveFarblingLevelFor(
      execution_context_, BraveFarblingLevel::OFF);
  // If Brave Shields are down or anti-fingerprinting is off for this site,
  // do nothing.
  if (farbling_level == BraveFarblingLevel::OFF)
    return;
  if (farbling_level == BraveFarblingLevel::MAXIMUM) {
    // If anti-fingerprinting is at maximum, override the entire language list
    // regardless of locale or other settings.
    languages_.clear();
    languages_.push_back("en-US");
    languages_.push_back("en");
  } else {
    // If anti-fingerprinting is on at its default level, remove all but the
    // first language. (Note: this method requires a non-empty list, which the
    // upstream code guarantees.)
    languages_.Shrink(1);
  }
}

}  // namespace blink
