/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_talk/brave_talk_side_panel_navigation_throttle.h"

#include <memory>

#include "brave/components/sidebar/browser/constants.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle_registry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"
#include "url/origin.h"

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveTalkSidePanelContents);

BraveTalkSidePanelContents::BraveTalkSidePanelContents(
    content::WebContents* contents)
    : content::WebContentsUserData<BraveTalkSidePanelContents>(*contents) {}

BraveTalkSidePanelContents::~BraveTalkSidePanelContents() = default;

// static
void BraveTalkSidePanelNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  content::NavigationHandle& handle = registry.GetNavigationHandle();

  // Only the panel's top-level document is pinned. The widget legitimately
  // embeds cross-origin conferencing subframes, so those are left alone.
  if (!handle.IsInOutermostMainFrame()) {
    return;
  }

  if (!BraveTalkSidePanelContents::FromWebContents(handle.GetWebContents())) {
    return;
  }

  registry.AddThrottle(
      std::make_unique<BraveTalkSidePanelNavigationThrottle>(registry));
}

BraveTalkSidePanelNavigationThrottle::BraveTalkSidePanelNavigationThrottle(
    content::NavigationThrottleRegistry& registry)
    : content::NavigationThrottle(registry) {}

BraveTalkSidePanelNavigationThrottle::~BraveTalkSidePanelNavigationThrottle() =
    default;

const char* BraveTalkSidePanelNavigationThrottle::GetNameForLogging() {
  return "BraveTalkSidePanelNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
BraveTalkSidePanelNavigationThrottle::WillStartRequest() {
  return MaybeOpenInMainWebContents();
}

content::NavigationThrottle::ThrottleCheckResult
BraveTalkSidePanelNavigationThrottle::WillRedirectRequest() {
  // Redirects are checked as well, otherwise an in-scope URL could be used to
  // walk the panel to an out-of-scope origin.
  return MaybeOpenInMainWebContents();
}

content::NavigationThrottle::ThrottleCheckResult
BraveTalkSidePanelNavigationThrottle::MaybeOpenInMainWebContents() {
  const GURL& url = navigation_handle()->GetURL();

  // In scope, so it stays in the panel. `url::Origin::Create` resolves `blob:`
  // and `filesystem:` URLs to their inner origin, so those stay too.
  if (url::Origin::Create(url).IsSameOriginWith(GURL(sidebar::kBraveTalkURL))) {
    return PROCEED;
  }

  // `about:blank` has an opaque origin but displays nothing of its own.
  if (url.IsAboutBlank()) {
    return PROCEED;
  }

  // Only web URLs are handed to the browser window. Anything else keeps its
  // existing handling: re-issuing it from the browser would turn a
  // renderer-initiated navigation into a browser-initiated one, which would
  // let the panel reach URLs a web renderer is not allowed to navigate to.
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return PROCEED;
  }

  auto* browser =
      webui::GetBrowserWindowInterface(navigation_handle()->GetWebContents());
  tabs::TabInterface* active_tab =
      browser ? browser->GetActiveTabInterface() : nullptr;
  if (!active_tab) {
    // Nothing to hand the navigation to. Drop it rather than let the panel
    // display an origin the user cannot see.
    return CANCEL_AND_IGNORE;
  }

  // Re-issued as a browser-initiated navigation of the active tab. The
  // referrer is preserved, but the opener relationship deliberately is not:
  // the panel should not retain script access to whatever it navigates away
  // to. `source_contents` must be set, otherwise `Navigate()` rewrites a
  // `CURRENT_TAB` disposition to `NEW_FOREGROUND_TAB`.
  NavigateParams params(browser, url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::CURRENT_TAB;
  params.source_contents = active_tab->GetContents();
  params.referrer = content::Referrer(navigation_handle()->GetReferrer());
  params.user_gesture = navigation_handle()->HasUserGesture();
  params.window_action = NavigateParams::WindowAction::kNoAction;
  Navigate(&params);

  return CANCEL_AND_IGNORE;
}
