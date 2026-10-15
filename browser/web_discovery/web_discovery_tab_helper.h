/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_
#define BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class PrefService;

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace web_discovery {

// Offers Web Discovery on Brave Search pages, and extracts page data for it.
// Owned by BraveTabFeatures.
class WebDiscoveryTabHelper : public tabs::ContentsObservingTabFeature {
 public:
  DECLARE_USER_DATA(WebDiscoveryTabHelper);

  static WebDiscoveryTabHelper* From(tabs::TabInterface* tab);

  // Returns nullptr unless the tab's profile is a regular profile.
  static std::unique_ptr<WebDiscoveryTabHelper> MaybeCreate(
      tabs::TabInterface& tab);

  explicit WebDiscoveryTabHelper(tabs::TabInterface& tab);

  ~WebDiscoveryTabHelper() override;

  WebDiscoveryTabHelper(const WebDiscoveryTabHelper&) = delete;
  WebDiscoveryTabHelper& operator=(const WebDiscoveryTabHelper&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(WebDiscoveryTest, InfobarAddedTest);

  // content::WebContentsObserver overrides:
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  void ShowInfoBar(PrefService* prefs);

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
  void MaybeExtractFromPage(content::RenderFrameHost* render_frame_host,
                            const GURL& url);
#endif

  ui::ScopedUnownedUserData<WebDiscoveryTabHelper> scoped_unowned_user_data_;
};

}  // namespace web_discovery

#endif  // BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_TAB_HELPER_H_
