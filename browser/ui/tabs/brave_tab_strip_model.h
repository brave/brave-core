/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STRIP_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STRIP_MODEL_H_

#include <cstdint>
#include <vector>

#include "base/callback_list.h"
#include "base/containers/span.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/tabs/public/brave_tab_strip_collection.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/prefs/pref_member.h"

class TreeTabModel;

class BraveTabStripModel : public TabStripModel {
 public:
  explicit BraveTabStripModel(TabStripModelDelegate* delegate,
                              Profile* profile,
                              TabGroupModelFactory* group_model_factory);

  ~BraveTabStripModel() override;

  BraveTabStripModel(const BraveTabStripModel&) = delete;
  BraveTabStripModel operator=(const BraveTabStripModel&) = delete;

  // Set the next tab when doing a MRU cycling with Ctrl-tab
  void SelectMRUTab(
      TabRelativeDirection direction,
      TabStripUserGestureDetails detail = TabStripUserGestureDetails(
          TabStripUserGestureDetails::GestureType::kOther));

  // Stop MRU cycling, called when releasing the Ctrl key
  void StopMRUCycling();

  // Exposes a |TabStripModel| api to |BraveTabMenuModel|.
  std::vector<int> GetTabIndicesForCommandAt(int tab_index);

  // Closes the tabs at the specified indices.
  void CloseTabs(
      base::span<int> indices,
      uint32_t close_flags = TabCloseTypes::CLOSE_CREATE_HISTORICAL_TAB);

  // Sets the custom title for the tab at the specified index.
  void SetCustomTitleForTab(int index,
                            const std::optional<std::u16string>& title);

  // Can be null when tree tab feature is disabled via flag or pref.
  const TreeTabModel* tree_model() const { return tree_tab_model_.get(); }
  TreeTabModel* tree_model() { return tree_tab_model_.get(); }

  // TabStripModel:
  void SelectRelativeTab(TabRelativeDirection direction,
                         TabStripUserGestureDetails detail) override;
  void UpdateWebContentsStateAt(int index, TabChangeType change_type) override;

 private:
  friend class TreeTabsBrowserTest;

  void OnTreeTabRelatedPrefChanged();
  void BuildTreeTabs();
  void FlattenTreeTabs();

  void NotifyTreeTabNodeCreated(const tabs::TreeTabNode& node);
  void NotifyTreeTabNodeDestroyed(const tree_tab::TreeTabNodeId& id);

  tabs::BraveTabStripCollection* contents_data() {
    return static_cast<tabs::BraveTabStripCollection*>(contents_data_.get());
  }

  tabs::TabStripCollection& GetTabStripCollectionForTesting();

  // List of tab indexes sorted by most recently used
  std::vector<int> mru_cycle_list_;

  BooleanPrefMember tree_tabs_enabled_;
  BooleanPrefMember vertical_tabs_enabled_;

  // The model for tree tabs hosted within this TabStripModel. When the feature
  // flag is disabled or the feature is turned off via related preferences,
  // this will be null.
  std::unique_ptr<TreeTabModel> tree_tab_model_;

  std::unique_ptr<base::CallbackListSubscription>
      tree_tab_node_created_subscription_;
  std::unique_ptr<base::CallbackListSubscription>
      tree_tab_node_destroyed_subscription_;
};

#endif  // BRAVE_BROWSER_UI_TABS_BRAVE_TAB_STRIP_MODEL_H_
