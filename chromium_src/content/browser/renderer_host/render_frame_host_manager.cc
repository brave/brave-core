/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

// Keeps the opener's effective URL for about:blank popups so Trezor Connect
// retains `window.opener` in its connect.trezor.io popup opened from
// chrome-untrusted://trezor-bridge. Templated as RenderFrameHostImpl is
// incomplete here.
template <typename RenderFrameHostImplT>
bool BraveShouldUseLastSuccessfulUrl(RenderFrameHostImplT* rfh,
                                     const GURL& dest_url) {
  const GURL& last_successful_url = rfh->last_successful_url();
  if (last_successful_url.is_empty()) {
    return false;
  }
  if (!last_successful_url.IsAboutBlank()) {
    return true;
  }
  const GURL& site_url = rfh->GetSiteInstance()->GetSiteInfo().site_url();
  return site_url.SchemeIs(content::kChromeUIUntrustedScheme) &&
         site_url.host() == kUntrustedTrezorHost &&
         dest_url.SchemeIs(url::kHttpsScheme) &&
         dest_url.host() == kTrezorConnectHost;
}

}  // namespace

#include <content/browser/renderer_host/render_frame_host_manager.cc>
