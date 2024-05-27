// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/web_discovery/browser/web_discovery_tab_helper.h"

#include "brave/components/web_discovery/browser/wdp_service.h"
#include "content/public/browser/navigation_handle.h"

namespace web_discovery {

WebDiscoveryTabHelper::WebDiscoveryTabHelper(content::WebContents* web_contents,
                                             WDPService* wdp_service)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<WebDiscoveryTabHelper>(*web_contents),
      wdp_service_(wdp_service) {
  CHECK(wdp_service);
}

WebDiscoveryTabHelper::~WebDiscoveryTabHelper() = default;

void WebDiscoveryTabHelper::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& url) {
  if (!render_frame_host->IsInPrimaryMainFrame()) {
    return;
  }
  wdp_service_->OnFinishNavigation(url, render_frame_host);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebDiscoveryTabHelper);

}  // namespace web_discovery
