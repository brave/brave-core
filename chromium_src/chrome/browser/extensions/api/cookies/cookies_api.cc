/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/api/cookies/cookies_api.h"

// This disables cookies.onChange routing in Tor windows (as it worked
// when there was a DCHECK instead of CHECK). Once crbug.com/417228685 is fixed
// in upstream, we'll be able to manage Tor profiles like any other main-OTR
// profile.

#define IsSerializeable(...)      \
  IsSerializeable(__VA_ARGS__) || \
      (otr && !profile_->GetPrimaryOTRProfile(/*create_if_needed=*/false))

#define OnOffTheRecordProfileCreated(...) \
  OnOffTheRecordProfileCreated_ChromiumImpl(__VA_ARGS__)

#include "src/chrome/browser/extensions/api/cookies/cookies_api.cc"

#undef IsSerializeable
#undef OnOffTheRecordProfileCreated

namespace extensions {

// static
void OnCookieChangeExposeForTesting::CallOnCookieChangeForOtr(
    CookiesAPI* cookies_api) {
  cookies_api->cookies_event_router_->OnCookieChange(true,
                                                     net::CookieChangeInfo());
}

void CookiesEventRouter::OnOffTheRecordProfileCreated(Profile* off_the_record) {
  if (off_the_record->IsTor()) {
    return;
  }

  OnOffTheRecordProfileCreated_ChromiumImpl(off_the_record);
}

}  // namespace extensions
