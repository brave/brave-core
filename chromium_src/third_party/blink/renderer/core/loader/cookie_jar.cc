/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/cookie_jar.h"

#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

bool CookieJar::ShouldUseEphemeralCookie() {
  if (!document_->GetFrame())
    return false;

  if (!document_->GetFrame()->IsCrossOriginToMainFrame())
    return false;

  return base::FeatureList::IsEnabled(blink::features::kBraveEphemeralStorage);
}

bool CookieJar::IsEphemeralCookieAllowed() {
  if (ShouldUseEphemeralCookie())
    return !ChromiumCookiesEnabled();
  return false;
}

bool CookieJar::CookiesEnabled() {
  bool cookies_enabled = ChromiumCookiesEnabled();
  if (!cookies_enabled)
    cookies_enabled = ShouldUseEphemeralCookie();

  return cookies_enabled;
}

}  // namespace blink

#define CookiesEnabled ChromiumCookiesEnabled
#include "../../../../../../../third_party/blink/renderer/core/loader/cookie_jar.cc"
#undef CookiesEnabled