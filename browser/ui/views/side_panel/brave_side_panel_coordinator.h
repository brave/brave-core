/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_COORDINATOR_H_

#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"

class BraveSidePanelCoordinator : public SidePanelCoordinator {
 public:
  using SidePanelCoordinator::SidePanelCoordinator;
  ~BraveSidePanelCoordinator() override;

  // SidePanelCoodinator overrides:
  void Show(absl::optional<SidePanelEntry::Id> entry_id = absl::nullopt,
            absl::optional<SidePanelUtil::SidePanelOpenTrigger> open_trigger =
                absl::nullopt) override;
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  std::unique_ptr<views::View> CreateHeader() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_COORDINATOR_H_
