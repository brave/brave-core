/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_TAB_HELPER_H_

#include <string>

#include "base/macros.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class Browser;

namespace brave_rewards {

class RewardsService;

class RewardsTabHelper : public RewardsServiceObserver,
                         public content::WebContentsObserver,
                         public BrowserListObserver,
                         public content::WebContentsUserData<RewardsTabHelper> {
 public:
  explicit RewardsTabHelper(content::WebContents*);
  ~RewardsTabHelper() override;

 private:
  friend class content::WebContentsUserData<RewardsTabHelper>;

  // content::WebContentsObserver overrides.
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void ResourceLoadComplete(
      content::RenderFrameHost* render_frame_host,
      const content::GlobalRequestID& request_id,
      const content::mojom::ResourceLoadInfo& resource_load_info) override;
  void DidAttachInterstitialPage() override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void WebContentsDestroyed() override;

#if !defined(OS_ANDROID)
  // BrowserListObserver overrides
  void OnBrowserSetLastActive(Browser* browser) override;
  void OnBrowserNoLongerActive(Browser* browser) override;
#endif

  SessionID tab_id_;
  RewardsService* rewards_service_;  // NOT OWNED

  WEB_CONTENTS_USER_DATA_KEY_DECL();
  DISALLOW_COPY_AND_ASSIGN(RewardsTabHelper);
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_TAB_HELPER_H_
