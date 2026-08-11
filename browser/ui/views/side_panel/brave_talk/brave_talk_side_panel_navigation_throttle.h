/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_TALK_BRAVE_TALK_SIDE_PANEL_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_TALK_BRAVE_TALK_SIDE_PANEL_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationThrottleRegistry;
class WebContents;
}  // namespace content

// Tags the `WebContents` hosted by `BraveTalkSidePanelWebView`. The side
// panel's contents is not a tab, so there is no `tabs::TabInterface` or tab
// helper to key off; this marker is how the navigation stack recognises it.
class BraveTalkSidePanelContents
    : public content::WebContentsUserData<BraveTalkSidePanelContents> {
 public:
  BraveTalkSidePanelContents(const BraveTalkSidePanelContents&) = delete;
  BraveTalkSidePanelContents& operator=(const BraveTalkSidePanelContents&) =
      delete;
  ~BraveTalkSidePanelContents() override;

 private:
  friend class content::WebContentsUserData<BraveTalkSidePanelContents>;

  explicit BraveTalkSidePanelContents(content::WebContents* contents);

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

// Pins the Brave Talk side panel to the `talk.brave.com` origin, routing
// anything else to the browser's active tab.
//
// This is enforced here rather than in `BraveTalkSidePanelWebView` because
// `content::WebContentsDelegate::OpenURLFromTab` is not consulted for ordinary
// same-frame navigations: Blink only defers those to the browser process for
// WebUI URLs and for non-current-tab dispositions, and otherwise starts them
// directly. A link click or `location.href =` in the widget therefore never
// reaches the delegate, and a `NavigationThrottle` is the only hook that both
// sees the URL and can stop the navigation.
//
// This matters because the side panel has no omnibox and no security
// indicator, so a user has no way to tell which origin it is showing.
class BraveTalkSidePanelNavigationThrottle
    : public content::NavigationThrottle {
 public:
  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry);

  explicit BraveTalkSidePanelNavigationThrottle(
      content::NavigationThrottleRegistry& registry);
  BraveTalkSidePanelNavigationThrottle(
      const BraveTalkSidePanelNavigationThrottle&) = delete;
  BraveTalkSidePanelNavigationThrottle& operator=(
      const BraveTalkSidePanelNavigationThrottle&) = delete;
  ~BraveTalkSidePanelNavigationThrottle() override;

  // content::NavigationThrottle:
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  ThrottleCheckResult MaybeOpenInMainWebContents();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_TALK_BRAVE_TALK_SIDE_PANEL_NAVIGATION_THROTTLE_H_
