/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_

#include "chrome/browser/ui/views/tabs/tab_container.h"

#include "chrome/browser/ui/views/tabs/tab_drag_context.h"

class BraveTabContainer : public TabContainer {
 public:
  METADATA_HEADER(BraveTabContainer);

  BraveTabContainer(TabStripController* controller,
                    TabHoverCardController* hover_card_controller,
                    TabDragContext* drag_context,
                    TabSlotController* tab_slot_controller,
                    views::View* scroll_contents_view);
  ~BraveTabContainer() override;

  // TabContainer:
  gfx::Size CalculatePreferredSize() const override;
  void UpdateClosingModeOnRemovedTab(int model_index, bool was_active) override;
  gfx::Rect GetTargetBoundsForClosingTab(Tab* tab,
                                         int former_model_index) const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_
