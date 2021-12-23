/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/onion_location_tab_helper.h"

namespace tor {

OnionLocationTabHelper::~OnionLocationTabHelper() = default;

// static
void OnionLocationTabHelper::SetOnionLocation(
    content::WebContents* web_contents,
    const GURL& onion_location) {
  OnionLocationTabHelper* tab_helper = FromWebContents(web_contents);
  if (!tab_helper)
    return;
  tab_helper->onion_location_ = onion_location;
}

OnionLocationTabHelper::OnionLocationTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsUserData<OnionLocationTabHelper>(*web_contents) {}

WEB_CONTENTS_USER_DATA_KEY_IMPL(OnionLocationTabHelper);

}  // namespace tor
