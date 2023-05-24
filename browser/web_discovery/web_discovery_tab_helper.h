/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_
#define BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_

#include "base/gtest_prod_util.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class PrefService;

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

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_
