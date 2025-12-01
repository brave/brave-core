/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/split_view/split_view_link_navigation_throttle.h"

#include "base/feature_list.h"
#include "chrome/browser/ui/ui_features.h"
#include "components/tabs/public/split_tab_collection.h"
#include "components/tabs/public/split_tab_data.h"
#include "components/tabs/public/split_tab_visual_data.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
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

  // Only handle split tabs.
  tabs::TabInterface* source_tab =
      tabs::TabInterface::MaybeGetFromContents(source);
  if (!source_tab || !source_tab->IsSplit()) {
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

  // Don't intercept POST requests (forms)
  if (navigation_handle()->IsPost()) {
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

  // Get the parent collection and cast to SplitTabCollection
  const tabs::TabCollection* parent_collection =
      source_tab->GetParentCollection();
  if (!parent_collection) {
    return PROCEED;
  }

  const tabs::SplitTabCollection* split_collection =
      static_cast<const tabs::SplitTabCollection*>(parent_collection);
  split_tabs::SplitTabData* split_data = split_collection->data();
  CHECK(split_data);
  if (!split_data->linked()) {
    return PROCEED;
  }

  // Get the list of tabs in the split and verify this is the left pane
  const std::vector<tabs::TabInterface*> tabs_in_split = split_data->ListTabs();
  if (tabs_in_split.size() != 2 || tabs_in_split[0]->GetContents() != source) {
    return PROCEED;
  }

  content::WebContents* target_contents = tabs_in_split[1]->GetContents();
  if (!target_contents) {
    return PROCEED;
  }

  // Load the URL in the right pane
  content::NavigationController::LoadURLParams params(
      navigation_handle()->GetURL());
  params.transition_type = ui::PAGE_TRANSITION_LINK;
  params.is_renderer_initiated = false;
  params.referrer = content::Referrer(navigation_handle()->GetReferrer());
  target_contents->GetController().LoadURLWithParams(params);

  return CANCEL;
}

const char* SplitViewLinkNavigationThrottle::GetNameForLogging() {
  return "SplitViewLinkNavigationThrottle";
}
