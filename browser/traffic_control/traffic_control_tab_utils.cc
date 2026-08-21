// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/traffic_control/traffic_control_tab_utils.h"

#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace traffic_control {

namespace {

bool IsEmptyOrNtpUrl(const GURL& url) {
  if (!url.is_valid() || url.IsAboutBlank() || url.spec() == "about:blank") {
    return true;
  }
  if (NewTabUI::IsNewTab(url)) {
    return true;
  }
  // Brave NTP uses chrome://newtab/ as well.
  return url.DeprecatedGetOriginAsURL() ==
         GURL(chrome::kChromeUINewTabURL).DeprecatedGetOriginAsURL();
}

}  // namespace

bool IsDiscardableEmptyTab(content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }

  // Prefer last committed; fall back to visible for mid-navigation NTP.
  const GURL& committed = web_contents->GetLastCommittedURL();
  const GURL& visible = web_contents->GetVisibleURL();
  if (!IsEmptyOrNtpUrl(committed) && !IsEmptyOrNtpUrl(visible)) {
    return false;
  }

  content::NavigationController& controller = web_contents->GetController();
  if (controller.CanGoBack()) {
    return false;
  }

  // Only the initial empty/NTP entry (or no entry yet).
  if (controller.GetEntryCount() > 1) {
    return false;
  }

  return true;
}

bool IsOmniboxNavigation(ui::PageTransition transition) {
  if (ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_TYPED)) {
    return true;
  }
  return (transition & ui::PAGE_TRANSITION_FROM_ADDRESS_BAR) != 0;
}

bool IsNewTabNavigation(content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }
  // Re-routing replaces the tab, which severs window.opener. Leave tabs that
  // script can still reach (window.open without noopener) alone.
  if (web_contents->HasOpener()) {
    return false;
  }
  return web_contents->GetController().IsInitialBlankNavigation();
}

}  // namespace traffic_control
