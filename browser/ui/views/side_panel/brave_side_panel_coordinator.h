/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_COORDINATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_COORDINATOR_H_

#include <memory>
#include <optional>

#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"

class BraveBrowserView;

class BraveSidePanelCoordinator : public SidePanelCoordinator {
 public:
  using SidePanelCoordinator::SidePanelCoordinator;
  ~BraveSidePanelCoordinator() override;

  // SidePanelCoodinator overrides:
  void Show(SidePanelEntry::Key entry_key,
            std::optional<SidePanelUtil::SidePanelOpenTrigger> open_trigger =
                std::nullopt) override;
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  std::unique_ptr<views::View> CreateHeader() override;
  void Toggle() override;
  void Toggle(SidePanelEntryKey key,
              SidePanelUtil::SidePanelOpenTrigger open_trigger) override;
  void OnViewVisibilityChanged(views::View* observed_view,
                               views::View* starting_from) override;
  void PopulateSidePanel(
      bool supress_animations,
      const UniqueKey& unique_key,
      SidePanelEntry* entry,
      std::optional<std::unique_ptr<views::View>> content_view) override;
  void NotifyPinnedContainerOfActiveStateChange(SidePanelEntryKey key,
                                                bool is_active) override;

 private:
  // Returns the last active entry or the default entry if no last active
  // entry exists.
  std::optional<SidePanelEntry::Key> GetLastActiveEntryKey() const;
  void UpdateToolbarButtonHighlight(bool side_panel_visible);
  BraveBrowserView* GetBraveBrowserView();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_COORDINATOR_H_
