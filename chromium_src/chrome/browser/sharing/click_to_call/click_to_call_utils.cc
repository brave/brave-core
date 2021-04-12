/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define ShouldOfferClickToCallForURL ShouldOfferClickToCallForURL_ChromiumImpl

#include "../../../../../../chrome/browser/sharing/click_to_call/click_to_call_utils.cc"

#undef ShouldOfferClickToCallForURL

bool ShouldOfferClickToCallForURL(content::BrowserContext* browser_context,
                                  const GURL& url) {
  return false;
}
