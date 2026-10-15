/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

// Trezor Connect relies on `window.opener` in its connect.trezor.io popup,
// opened blank from chrome-untrusted://trezor-bridge and then navigated.
// Templated as RenderFrameHostImpl is incomplete here.
template <typename RenderFrameHostImplT>
bool BraveShouldUseLastSuccessfulUrl(RenderFrameHostImplT* rfh,
                                     const GURL& dest_url) {
  if (!rfh->last_successful_url().IsAboutBlank()) {
    return false;
  }
  const GURL& site_url = rfh->GetSiteInstance()->GetSiteInfo().site_url();
  return site_url.SchemeIs(content::kChromeUIUntrustedScheme) &&
         site_url.host() == kUntrustedTrezorHost &&
         dest_url.SchemeIs(url::kHttpsScheme) &&
         dest_url.host() == kTrezorConnectHost;
}

}  // namespace

#include <content/browser/renderer_host/render_frame_host_manager.cc>
