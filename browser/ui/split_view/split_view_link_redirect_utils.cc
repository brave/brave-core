/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/split_view/split_view_link_redirect_utils.h"

#include <optional>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "components/tabs/public/split_tab_collection.h"
#include "components/tabs/public/split_tab_data.h"
#include "components/tabs/public/split_tab_id.h"
#include "components/tabs/public/tab_collection.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/referrer.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace {

// WebContentsUserData to temporarily store the split tab ID for redirect
// purposes. This is used when a new WebContents is created (e.g., from
// window.open) and needs to be redirected based on its source split view.
class SplitTabIdData : public content::WebContentsUserData<SplitTabIdData> {
 public:
  ~SplitTabIdData() override = default;

  const split_tabs::SplitTabId& split_tab_id() const { return split_tab_id_; }

 private:
  friend class content::WebContentsUserData<SplitTabIdData>;

  explicit SplitTabIdData(content::WebContents* contents,
                          const split_tabs::SplitTabId& split_tab_id)
      : content::WebContentsUserData<SplitTabIdData>(*contents),
        split_tab_id_(split_tab_id) {}

  split_tabs::SplitTabId split_tab_id_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(SplitTabIdData);

// Gets and clears the split tab ID from a WebContents. Returns nullopt if
// no split tab ID was set. This is a one-time read operation.
std::optional<split_tabs::SplitTabId> GetAndClearSplitTabIdForRedirect(
    content::WebContents* contents) {
  CHECK(contents);
  SplitTabIdData* data = SplitTabIdData::FromWebContents(contents);
  if (!data) {
    return std::nullopt;
  }

  split_tabs::SplitTabId split_tab_id = data->split_tab_id();
  // Clear the data after reading it (one-time use)
  contents->RemoveUserData(SplitTabIdData::UserDataKey());
  return split_tab_id;
}

content::WebContents* GetRightPaneIfLinked(
    content::WebContents* source,
    bool* out_from_window_open = nullptr) {
  CHECK(source);

  // First, check if this WebContents has a split tab ID for redirect
  // purposes (e.g., from window.open). If so, clear it and use it to find
  // the right pane.
  std::optional<split_tabs::SplitTabId> split_tab_id =
      GetAndClearSplitTabIdForRedirect(source);
  if (split_tab_id.has_value()) {
    if (out_from_window_open) {
      *out_from_window_open = true;
    }

    // Get the TabStripModel to look up the split data
    tabs::TabInterface* source_tab =
        tabs::TabInterface::MaybeGetFromContents(source);
    if (!source_tab) {
      return nullptr;
    }

    TabStripModel* tab_strip_model =
        source_tab->GetBrowserWindowInterface()->GetTabStripModel();
    if (!tab_strip_model) {
      return nullptr;
    }

    split_tabs::SplitTabData* split_data =
        tab_strip_model->GetSplitData(split_tab_id.value());
    if (!split_data || !split_data->linked()) {
      return nullptr;
    }

    // Get the right pane (second tab in the split)
    const std::vector<tabs::TabInterface*> tabs_in_split =
        split_data->ListTabs();
    if (tabs_in_split.size() != 2) {
      return nullptr;
    }

    return tabs_in_split[1]->GetContents();
  }

  if (out_from_window_open) {
    *out_from_window_open = false;
  }

  // Original logic: check if source is already in a split view
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

  bool from_window_open = false;
  content::WebContents* target_contents =
      GetRightPaneIfLinked(source, &from_window_open);
  if (!target_contents) {
    return false;
  }

  // We can load url via target contents' NavigationController
  // but Navigate() is definitely the better choice as this is
  // another browser initiated navigation.
  tabs::TabInterface* target_tab =
      tabs::TabInterface::GetFromContents(target_contents);
  NavigateParams params(target_tab->GetBrowserWindowInterface(), url,
                        ui::PAGE_TRANSITION_LINK);
  params.source_contents = target_contents;
  params.disposition = WindowOpenDisposition::CURRENT_TAB;
  // Preserve original navigation's referrer.
  // As this redirects to existing right pane, orignal navigation's opener
  // can't be passed because this opener relationship is established only
  // when new WebContents is created.
  params.referrer = referrer;
  Navigate(&params);

  // Close the source tab if it was created by window.open and we redirected.
  // We need to close it asynchronously after the current call stack completes
  // to avoid interfering with the navigation system.
  if (from_window_open) {
    tabs::TabInterface* source_tab =
        tabs::TabInterface::MaybeGetFromContents(source);
    if (source_tab) {
      base::WeakPtr<tabs::TabInterface> weak_tab = source_tab->GetWeakPtr();
      base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE, base::BindOnce(
                         [](base::WeakPtr<tabs::TabInterface> tab) {
                           if (tab) {
                             tab->Close();
                           }
                         },
                         weak_tab));
    }
  }

  return true;
}

bool SetSplitTabIdForRedirect(content::WebContents* source,
                              content::WebContents* new_contents) {
  CHECK(source && new_contents);

  // Get the split tab ID from the source
  tabs::TabInterface* source_tab =
      tabs::TabInterface::MaybeGetFromContents(source);
  if (!source_tab) {
    return false;
  }

  std::optional<split_tabs::SplitTabId> split_tab_id = source_tab->GetSplit();
  if (!split_tab_id.has_value()) {
    return false;
  }

  // Verify this is the left pane and the split is linked
  const tabs::TabCollection* parent_collection =
      source_tab->GetParentCollection();
  if (!parent_collection) {
    return false;
  }

  const tabs::SplitTabCollection* split_collection =
      static_cast<const tabs::SplitTabCollection*>(parent_collection);
  split_tabs::SplitTabData* split_data = split_collection->data();
  if (!split_data || !split_data->linked()) {
    return false;
  }

  const std::vector<tabs::TabInterface*> tabs_in_split = split_data->ListTabs();
  if (tabs_in_split.size() != 2 || tabs_in_split[0]->GetContents() != source) {
    return false;
  }

  // This is the left pane of a linked split view, set the split tab ID
  SplitTabIdData::CreateForWebContents(new_contents, split_tab_id.value());
  return true;
}

}  // namespace split_view
