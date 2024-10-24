/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_
#define BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_

#include "base/gtest_prod_util.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
#include "base/memory/raw_ptr.h"
#include "brave/components/web_discovery/browser/web_discovery_service.h"
#endif

class PrefService;

namespace web_discovery {

class WebDiscoveryTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebDiscoveryTabHelper> {
 public:
  static void MaybeCreateForWebContents(content::WebContents* contents);

  ~WebDiscoveryTabHelper() override;

  WebDiscoveryTabHelper(const WebDiscoveryTabHelper&) = delete;
  WebDiscoveryTabHelper& operator=(const WebDiscoveryTabHelper&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(WebDiscoveryTest, InfobarAddedTest);

  friend class content::WebContentsUserData<WebDiscoveryTabHelper>;

  explicit WebDiscoveryTabHelper(content::WebContents* contents);

  // content::WebContentsObserver overrides:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  void ShowInfoBar(PrefService* prefs);

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
  void MaybeExtractFromPage(content::RenderFrameHost* render_frame_host,
                            const GURL& url);

  raw_ptr<WebDiscoveryService> web_discovery_service_ = nullptr;
#endif

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace web_discovery

#endif  // BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_
