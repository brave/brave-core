/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/common/url_constants.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

// Trezor Connect needs `window.opener` in its connect.trezor.io popup opened
// from chrome-untrusted://trezor-bridge.
bool BraveIsTrezorPopup(const GURL& last_successful_url,
                        const GURL& site_url,
                        const GURL& dest_url) {
  return last_successful_url.IsAboutBlank() &&
         site_url.SchemeIs(content::kChromeUIUntrustedScheme) &&
         site_url.host() == "trezor-bridge" &&
         dest_url.SchemeIs(url::kHttpsScheme) &&
         dest_url.host() == "connect.trezor.io";
}

}  // namespace

#include <content/browser/renderer_host/render_frame_host_manager.cc>
