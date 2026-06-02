// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_TREE_TAB_SESSION_OBSERVER_H_
#define BRAVE_BROWSER_UI_TABS_TREE_TAB_SESSION_OBSERVER_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/sessions/core/session_id.h"

class Profile;
class TabStripModel;

namespace tabs {
class TreeTabNode;
}  // namespace tabs

// Observes a TabStripModel's tree tab notifications and writes the resulting
// tree-structure changes to SessionService so that the hierarchy can be
// restored across browser restarts and via "restore closed tab".
//
// Mirrors the role of SessionServiceTabGroupSyncObserver for tab groups: it
// translates per-window tree node changes into SessionService::AddTabExtraData
// calls keyed on this browser's SessionID.
class TreeTabSessionObserver : public TabStripModelObserver {
 public:
  TreeTabSessionObserver(Profile* profile,
                         TabStripModel* tab_strip_model,
                         SessionID session_id);
  TreeTabSessionObserver(const TreeTabSessionObserver&) = delete;
  TreeTabSessionObserver& operator=(const TreeTabSessionObserver&) = delete;
  ~TreeTabSessionObserver() override;

  // Called from BrowserLiveTabContext::GetExtraDataForTab to record the
  // tree-tab position of the tab being closed. Writes kBraveTreeNodeIdKey and
  // kBraveTreeParentNodeIdKey, and kBraveTreeNodeCollapsedKey into
  // |extra_data|.
  void MaybePopulateTreeTabExtraData(
      int index,
      std::map<std::string, std::string>* extra_data);

 private:
  // TabStripModelObserver:
  void OnTreeTabChanged(const TreeTabChange& change) override;

  // Pushes the full tree-position data (node id, parent id, collapsed state)
  // for every tab owned by |node| to SessionService.
  void UpdateTreeTabSessionDataForNode(const tabs::TreeTabNode& node);

  // Pushes only the collapsed state for every tab owned by |node| to
  // SessionService.
  void UpdateTreeTabCollapsedState(const tabs::TreeTabNode& node);

  raw_ptr<Profile> profile_;
  raw_ptr<TabStripModel> tab_strip_model_;
  const SessionID session_id_;
};

#endif  // BRAVE_BROWSER_UI_TABS_TREE_TAB_SESSION_OBSERVER_H_
