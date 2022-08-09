/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/navigator_language.h"

#include "third_party/blink/public/platform/web_content_settings_client.h"

#define NavigatorLanguage NavigatorLanguage_ChromiumImpl
#include "src/third_party/blink/renderer/core/frame/navigator_language.cc"
#undef NavigatorLanguage

namespace blink {

NavigatorLanguage::NavigatorLanguage(ExecutionContext* execution_context)
    : NavigatorLanguage_ChromiumImpl(execution_context) {}

void NavigatorLanguage::EnsureUpdatedLanguage() {
  NavigatorLanguage_ChromiumImpl::EnsureUpdatedLanguage();
  blink::WebContentSettingsClient* settings =
      brave::GetContentSettingsClientFor(execution_context_);
  // If "reduce language" feature flag is off or "reduce language" profile-level
  // preference is toggled off or Brave Shields are down for this site or
  // anti-fingerprinting is off for this site, do nothing.
  if (!settings || !settings->IsReduceLanguageEnabled() ||
      settings->GetBraveFarblingLevel() == BraveFarblingLevel::OFF)
    return;
  if (settings->GetBraveFarblingLevel() == BraveFarblingLevel::MAXIMUM) {
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
