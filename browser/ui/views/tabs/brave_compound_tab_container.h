/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_COMPOUND_TAB_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_COMPOUND_TAB_CONTAINER_H_

#include "chrome/browser/ui/views/tabs/compound_tab_container.h"

class BraveCompoundTabContainer : public CompoundTabContainer {
 public:
  BraveCompoundTabContainer(raw_ref<TabContainerController> controller,
                            TabHoverCardController* hover_card_controller,
                            TabDragContextBase* drag_context,
                            TabSlotController& tab_slot_controller,
                            views::View* scroll_contents_view);
  ~BraveCompoundTabContainer() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_COMPOUND_TAB_CONTAINER_H_
