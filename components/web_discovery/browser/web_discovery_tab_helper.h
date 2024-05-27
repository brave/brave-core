/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WEB_DISCOVERY_TAB_HELPER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WEB_DISCOVERY_TAB_HELPER_H_

#include "base/memory/raw_ptr.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace web_discovery {

class WDPService;

class WebDiscoveryTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebDiscoveryTabHelper> {
 public:
  WebDiscoveryTabHelper(content::WebContents* web_contents,
                        WDPService* wdp_service);
  ~WebDiscoveryTabHelper() override;

  WebDiscoveryTabHelper(const WebDiscoveryTabHelper&) = delete;
  WebDiscoveryTabHelper& operator=(const WebDiscoveryTabHelper&) = delete;

 private:
  friend class content::WebContentsUserData<WebDiscoveryTabHelper>;

  // content::WebContentsObserver:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& url) override;

  raw_ptr<WDPService> wdp_service_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_WEB_DISCOVERY_TAB_HELPER_H_
