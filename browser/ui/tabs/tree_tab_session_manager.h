// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_TREE_TAB_SESSION_MANAGER_H_
#define BRAVE_BROWSER_UI_TABS_TREE_TAB_SESSION_MANAGER_H_

#include <map>
#include <string>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/sessions/core/session_id.h"
#include "components/tabs/public/tab_interface.h"

class Profile;
class TabStripModel;

namespace tabs {
class TabInterface;
class TreeTabNodeTabCollection;
class TreeTabNode;
}  // namespace tabs

// Observes a TabStripModel's tree tab notifications and writes the resulting
// tree-structure changes to SessionService so that the hierarchy can be
// restored across browser restarts and via "restore closed tab".
//
// Mirrors the role of SessionServiceTabGroupSyncObserver for tab groups: it
// translates per-window tree node changes into SessionService::AddTabExtraData
// calls keyed on this browser's SessionID.
class TreeTabSessionManager : public TabStripModelObserver {
 public:
  TreeTabSessionManager(Profile* profile,
                        TabStripModel* tab_strip_model,
                        SessionID session_id);
  TreeTabSessionManager(const TreeTabSessionManager&) = delete;
  TreeTabSessionManager& operator=(const TreeTabSessionManager&) = delete;
  ~TreeTabSessionManager() override;

  // Called from BrowserLiveTabContext::GetExtraDataForTab to record the
  // tree-tab position of the tab being closed. Writes kBraveTreeNodeIdKey and
  // kBraveTreeParentNodeIdKey, and kBraveTreeNodeCollapsedKey into
  // |extra_data|.
  void MaybePopulateTreeTabExtraData(
      int index,
      std::map<std::string, std::string>* extra_data);

  // Called from BrowserLiveTabContext::AddRestoredTab after the tab has been
  // re-inserted into the browser. Reads kBraveTreeParentNodeIdKey from
  // |extra_data| and, if the parent tree node is still present in the strip,
  // reparents the restored tab's TreeTabNodeTabCollection under it.
  void MaybeRestoreTabTreeHierarchy(
      content::WebContents* restored_web_contents,
      const std::map<std::string, std::string>& extra_data);

 private:
  // TabStripModelObserver:
  void OnTreeTabChanged(const TreeTabChange& change) override;

  // Pushes the full tree-position data (node id, parent id, collapsed state)
  // for every tab owned by |node| to SessionService.
  void UpdateTreeTabSessionDataForNode(const tabs::TreeTabNode& node);

  // Pushes only the collapsed state for every tab owned by |node| to
  // SessionService.
  void UpdateTreeTabCollapsedState(const tabs::TreeTabNode& node);

  // Try restoring group's tree hierarchy for the restored tab. This is called
  // when the restored tab is not a direct child of a tree node tab, but is in a
  // group.
  void MaybeRestoreGroupTreeHierarchy(
      tabs::TabInterface* restored_tab,
      const std::map<std::string, std::string>& extra_data);

  // Try restoring split's tree hierarchy for the restored tab. This is called
  // when the restored tab is not a direct child of a tree node tab, but is in a
  // split.
  void MaybeRestoreSplitTreeHierarchy(
      tabs::TabInterface* restored_tab,
      const std::map<std::string, std::string>& extra_data);

  // Restores the tree hierarchy for the restored tab. Restores id, parent id,
  // and collapsed state for the tab's TreeTabNodeTabCollection.
  void RestoreTreeTabNodeCollection(
      tabs::TreeTabNodeTabCollection* tree_coll,
      const std::map<std::string, std::string>& extra_data);

  raw_ptr<Profile> profile_;
  raw_ptr<TabStripModel> tab_strip_model_;
  const SessionID session_id_;
};

#endif  // BRAVE_BROWSER_UI_TABS_TREE_TAB_SESSION_MANAGER_H_
