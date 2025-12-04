/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/split_view/split_view_link_navigation_throttle.h"

#include "base/feature_list.h"
#include "brave/browser/ui/split_view/split_view_link_redirect_utils.h"
#include "chrome/browser/ui/ui_features.h"
#include "content/public/browser/navigation_handle.h"
#include "net/http/http_request_headers.h"
#include "ui/base/window_open_disposition.h"

// static
void SplitViewLinkNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {
  if (!base::FeatureList::IsEnabled(features::kSideBySide)) {
    return;
  }

  registry.AddThrottle(
      std::make_unique<SplitViewLinkNavigationThrottle>(registry));
}

SplitViewLinkNavigationThrottle::SplitViewLinkNavigationThrottle(
    content::NavigationThrottleRegistry& registry)
    : content::NavigationThrottle(registry) {}

SplitViewLinkNavigationThrottle::~SplitViewLinkNavigationThrottle() = default;

content::NavigationThrottle::ThrottleCheckResult
SplitViewLinkNavigationThrottle::WillStartRequest() {
  return MaybeRedirectToRightPane();
}

content::NavigationThrottle::ThrottleCheckResult
SplitViewLinkNavigationThrottle::MaybeRedirectToRightPane() {
  content::WebContents* source = navigation_handle()->GetWebContents();
  if (!source) {
    return PROCEED;
  }

  // Only handle main frame navigations
  if (!navigation_handle()->IsInMainFrame()) {
    return PROCEED;
  }

  // Only intercept user-initiated navigations.
  if (!navigation_handle()->HasUserGesture()) {
    return PROCEED;
  }

  // Only intercept renderer-initiated navigations.
  if (!navigation_handle()->IsRendererInitiated()) {
    return PROCEED;
  }

  // Only intercept GET requests as it carries all data in the URL.
  if (navigation_handle()->GetRequestMethod() !=
      net::HttpRequestHeaders::kGetMethod) {
    return PROCEED;
  }

  // Don't intercept same-document navigations
  if (navigation_handle()->IsSameDocument()) {
    return PROCEED;
  }

  // Don't intercept downloads
  if (navigation_handle()->IsDownload()) {
    return PROCEED;
  }

  // Use the shared redirect manager to check and perform redirect
  if (split_view::MaybeRedirectToRightPane(
          source, navigation_handle()->GetURL(),
          content::Referrer(navigation_handle()->GetReferrer()))) {
    return CANCEL;
  }

  return PROCEED;
}

const char* SplitViewLinkNavigationThrottle::GetNameForLogging() {
  return "SplitViewLinkNavigationThrottle";
}
