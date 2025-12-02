/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/split_view/split_view_link_redirect_utils.h"

#include "chrome/browser/ui/ui_features.h"
#include "components/tabs/public/split_tab_collection.h"
#include "components/tabs/public/split_tab_data.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace {

content::WebContents* GetRightPaneIfLinked(content::WebContents* source) {
  if (!source) {
    return nullptr;
  }

  // Only handle split tabs.
  tabs::TabInterface* source_tab =
      tabs::TabInterface::MaybeGetFromContents(source);
  if (!source_tab || !source_tab->IsSplit()) {
    return nullptr;
  }

  // Get the parent collection and cast to SplitTabCollection
  const tabs::TabCollection* parent_collection =
      source_tab->GetParentCollection();
  if (!parent_collection) {
    return nullptr;
  }

  const tabs::SplitTabCollection* split_collection =
      static_cast<const tabs::SplitTabCollection*>(parent_collection);
  split_tabs::SplitTabData* split_data = split_collection->data();
  if (!split_data || !split_data->linked()) {
    return nullptr;
  }

  // Get the list of tabs in the split and verify this is the left pane
  const std::vector<tabs::TabInterface*> tabs_in_split = split_data->ListTabs();
  if (tabs_in_split.size() != 2 || tabs_in_split[0]->GetContents() != source) {
    return nullptr;
  }

  return tabs_in_split[1]->GetContents();
}

}  // namespace

namespace split_view {

bool MaybeRedirectToRightPane(content::WebContents* source,
                              const GURL& url,
                              const content::Referrer& referrer) {
  if (!base::FeatureList::IsEnabled(features::kSideBySide)) {
    return false;
  }

  if (!source) {
    return false;
  }

  content::WebContents* target_contents = GetRightPaneIfLinked(source);
  if (!target_contents) {
    return false;
  }

  // Load the URL in the right pane
  content::NavigationController::LoadURLParams params(url);
  params.transition_type = ui::PAGE_TRANSITION_LINK;
  params.is_renderer_initiated = false;
  params.referrer = referrer;
  target_contents->GetController().LoadURLWithParams(params);

  return true;
}

}  // namespace split_view
