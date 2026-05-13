// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_SESSIONS_BRAVE_SESSION_TREE_TAB_HELPER_H_
#define BRAVE_BROWSER_SESSIONS_BRAVE_SESSION_TREE_TAB_HELPER_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class Browser;
class SessionService;
class TabStripModel;

// BraveSessionTreeTabHelper observes tree-tab structure changes on a single
// browser window and schedules kCommandAddTabExtraData commands on the
// SessionService so that the parent-child hierarchy and collapsed state of
// every TreeTabNode are persisted incrementally between full rebuilds.
//
// One instance is created per browser window when the kBraveTreeTab feature
// is enabled. The helper is owned by the SessionService overlay code and
// lives as long as the browser's TabStripModel.
class BraveSessionTreeTabHelper : public TabStripModelObserver {
 public:
  BraveSessionTreeTabHelper(Browser* browser, SessionService* session_service);
  ~BraveSessionTreeTabHelper() override;

  BraveSessionTreeTabHelper(const BraveSessionTreeTabHelper&) = delete;
  BraveSessionTreeTabHelper& operator=(const BraveSessionTreeTabHelper&) =
      delete;

  // Schedules kCommandAddTabExtraData commands for every tree node currently
  // present in the browser. Called from the BuildCommandsForBrowser path to
  // ensure the full tree state is written during session rebuilds.
  void ScheduleAllTreeNodeCommands();

  // TabStripModelObserver:
  void OnTreeTabChanged(const TreeTabChange& change) override;
  void OnTabStripModelDestroyed(TabStripModel* tab_strip_model) override;

 private:
  // Schedules the three extra-data keys for the given tree node.
  void ScheduleTreeNodeExtraData(const tabs::TreeTabNode& node);

  // Schedules only the collapsed-state key for the given tree node.
  void ScheduleCollapsedStateExtraData(const tabs::TreeTabNode& node);

  raw_ptr<Browser> browser_;
  raw_ptr<SessionService> session_service_;
};

#endif  // BRAVE_BROWSER_SESSIONS_BRAVE_SESSION_TREE_TAB_HELPER_H_
