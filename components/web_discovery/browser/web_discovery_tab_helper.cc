// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/web_discovery/browser/web_discovery_tab_helper.h"

#include <utility>

#include "brave/components/web_discovery/browser/web_discovery_service.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace web_discovery {

WebDiscoveryTabHelper::WebDiscoveryTabHelper(
    content::WebContents* web_contents,
    WebDiscoveryService* web_discovery_service)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<WebDiscoveryTabHelper>(*web_contents),
      web_discovery_service_(web_discovery_service) {
  CHECK(web_discovery_service);
}

WebDiscoveryTabHelper::~WebDiscoveryTabHelper() = default;

void WebDiscoveryTabHelper::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& url) {
  if (!render_frame_host->IsInPrimaryMainFrame()) {
    return;
  }
  if (!web_discovery_service_->ShouldExtractFromPage(url, render_frame_host)) {
    return;
  }
  mojo::Remote<mojom::DocumentExtractor> remote;
  render_frame_host->GetRemoteInterfaces()->GetInterface(
      remote.BindNewPipeAndPassReceiver());
  web_discovery_service_->StartExtractingFromPage(url, std::move(remote));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebDiscoveryTabHelper);

}  // namespace web_discovery
