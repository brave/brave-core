/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>

#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/brave_tree_tab_strip_collection_delegate.h"
#include "brave/browser/ui/tabs/tree_tab_model.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/tabs/public/brave_tab_strip_collection.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/tabs/public/tree_tab_node_tab_collection.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tab_ui_helper.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_strip_collection.h"
#include "components/tabs/public/unpinned_tab_collection.h"
#include "content/public/browser/web_contents.h"

BraveTabStripModel::BraveTabStripModel(
    TabStripModelDelegate* delegate,
    Profile* profile,
    TabGroupModelFactory* group_model_factory)
    : TabStripModel(delegate, profile, group_model_factory) {
  if (base::FeatureList::IsEnabled(tabs::kBraveTreeTab) &&
      delegate->IsNormalWindow()) {
    // Replace the default TabStripCollection with Brave's version
    contents_data_ = std::make_unique<tabs::BraveTabStripCollection>();

    tree_tabs_enabled_.Init(
        brave_tabs::kTreeTabsEnabled, profile->GetPrefs(),
        base::BindRepeating(&BraveTabStripModel::OnTreeTabRelatedPrefChanged,
                            base::Unretained(this)));
    vertical_tabs_enabled_.Init(
        brave_tabs::kVerticalTabsEnabled, profile->GetPrefs(),
        base::BindRepeating(&BraveTabStripModel::OnTreeTabRelatedPrefChanged,
                            base::Unretained(this)));
    OnTreeTabRelatedPrefChanged();
  }
}

BraveTabStripModel::~BraveTabStripModel() = default;

void BraveTabStripModel::SelectRelativeTab(TabRelativeDirection direction,
                                           TabStripUserGestureDetails detail) {
  if (GetTabCount() == 0) {
    return;
  }

  bool is_mru_enabled = profile()->GetPrefs()->GetBoolean(kMRUCyclingEnabled);

  if (is_mru_enabled) {
    SelectMRUTab(direction, detail);
  } else {
    TabStripModel::SelectRelativeTab(direction, detail);
  }
}

void BraveTabStripModel::UpdateWebContentsStateAt(int index,
                                                  TabChangeType change_type) {
  if (base::FeatureList::IsEnabled(tabs::kBraveRenamingTabs)) {
    // Make sure that the tab's last origin is updated when the url changes.
    // When last origin changes, the custom title is reset.
    GetTabAtIndex(index)->GetTabFeatures()->tab_ui_helper()->UpdateLastOrigin();
  }

  TabStripModel::UpdateWebContentsStateAt(index, change_type);
}

void BraveTabStripModel::SelectMRUTab(TabRelativeDirection direction,
                                      TabStripUserGestureDetails detail) {
  if (mru_cycle_list_.empty()) {
    // Start cycling

    Browser* browser = chrome::FindBrowserWithTab(GetWebContentsAt(0));
    if (!browser) {
      return;
    }

    // Create a list of tab indexes sorted by time of last activation
    for (int i = 0; i < count(); ++i) {
      mru_cycle_list_.push_back(i);
    }

    std::sort(mru_cycle_list_.begin(), mru_cycle_list_.end(),
              [this](int a, int b) {
                return GetWebContentsAt(a)->GetLastActiveTimeTicks() >
                       GetWebContentsAt(b)->GetLastActiveTimeTicks();
              });

    // Tell the cycling controller that we start cycling to handle tabs keys
    static_cast<BraveBrowserWindow*>(browser->window())->StartTabCycling();
  }

  if (direction == TabRelativeDirection::kNext) {
    std::rotate(mru_cycle_list_.begin(), mru_cycle_list_.begin() + 1,
                mru_cycle_list_.end());
  } else {
    std::rotate(mru_cycle_list_.rbegin(), mru_cycle_list_.rbegin() + 1,
                mru_cycle_list_.rend());
  }

  ActivateTabAt(mru_cycle_list_[0], detail);
}

void BraveTabStripModel::StopMRUCycling() {
  mru_cycle_list_.clear();
}

std::vector<int> BraveTabStripModel::GetTabIndicesForCommandAt(int tab_index) {
  return TabStripModel::GetIndicesForCommand(tab_index);
}

void BraveTabStripModel::CloseTabs(base::span<int> indices,
                                   uint32_t close_types) {
  std::vector<content::WebContents*> contentses;
  for (const auto& index : indices) {
    contentses.push_back(GetWebContentsAt(index));
  }
  TabStripModel::CloseTabs(contentses, close_types);
}

void BraveTabStripModel::SetCustomTitleForTab(
    int index,
    const std::optional<std::u16string>& title) {
  CHECK(base::FeatureList::IsEnabled(tabs::kBraveRenamingTabs));

  auto* tab_interface = GetTabAtIndex(index);
  CHECK(tab_interface);
  auto* tab_ui_helper = tab_interface->GetTabFeatures()->tab_ui_helper();
  CHECK(tab_ui_helper);
  tab_ui_helper->SetCustomTitle(title);

  for (auto& observer : observers_) {
    observer.TabCustomTitleChanged(
        GetWebContentsAt(index),
        title.has_value() ? base::UTF16ToUTF8(*title) : std::string());
  }

  NotifyTabChanged(tab_interface, TabChangeType::kAll);
}

void BraveTabStripModel::OnTreeTabRelatedPrefChanged() {
  if (*tree_tabs_enabled_ && *vertical_tabs_enabled_) {
    BuildTreeTabs();
    tree_tab_node_created_subscription_ =
        std::make_unique<base::CallbackListSubscription>(
            tree_tab_model_->RegisterAddTreeTabNodeCallback(base::BindRepeating(
                &BraveTabStripModel::NotifyTreeTabNodeCreated,
                base::Unretained(this))));
    tree_tab_node_destroyed_subscription_ =
        std::make_unique<base::CallbackListSubscription>(
            tree_tab_model_->RegisterRemoveTreeTabNodeCallback(
                base::BindRepeating(
                    &BraveTabStripModel::NotifyTreeTabNodeDestroyed,
                    base::Unretained(this))));
  } else {
    tree_tab_node_created_subscription_.reset();
    tree_tab_node_destroyed_subscription_.reset();
    FlattenTreeTabs();
  }
}

void BraveTabStripModel::BuildTreeTabs() {
  CHECK(base::FeatureList::IsEnabled(tabs::kBraveTreeTab));
  CHECK(!tree_tab_model_);
  tree_tab_model_ = std::make_unique<TreeTabModel>();

  contents_data()->SetDelegate(
      std::make_unique<BraveTreeTabStripCollectionDelegate>(
          *contents_data(), tree_tab_model_->GetWeakPtr()));
}

void BraveTabStripModel::FlattenTreeTabs() {
  CHECK(base::FeatureList::IsEnabled(tabs::kBraveTreeTab));

  if (!tree_tab_model_) {
    return;
  }

  contents_data()->SetDelegate(nullptr);
}

void BraveTabStripModel::NotifyTreeTabNodeCreated(
    const tabs::TreeTabNode& node) {
  auto change = TreeTabChange(node.id(), TreeTabChange::CreatedChange(node));
  for (auto& observer : observers_) {
    observer.OnTreeTabChanged(change);
  }
}

void BraveTabStripModel::NotifyTreeTabNodeDestroyed(
    const tree_tab::TreeTabNodeId& id) {
  auto* node = tree_tab_model_->GetNode(id);
  CHECK(node);
  auto change = TreeTabChange(id, TreeTabChange::WillBeDestroyedChange(*node));
  for (auto& observer : observers_) {
    observer.OnTreeTabChanged(change);
  }
}

tabs::TabStripCollection&
BraveTabStripModel::GetTabStripCollectionForTesting() {
  return *contents_data();
}
