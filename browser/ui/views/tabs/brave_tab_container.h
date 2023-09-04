/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_

#include "chrome/browser/ui/views/tabs/tab_container_impl.h"

#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/tab_drag_context.h"

class BraveTabContainer : public TabContainerImpl {
 public:
  METADATA_HEADER(BraveTabContainer);

  BraveTabContainer(TabContainerController& controller,
                    TabHoverCardController* hover_card_controller,
                    TabDragContextBase* drag_context,
                    TabSlotController& tab_slot_controller,
                    views::View* scroll_contents_view);
  ~BraveTabContainer() override;

  // Calling this will freeze this view's layout. When the returned closure
  // runs, layout will be unlocked and run immediately.
  // This is to avoid accessing invalid index during reconstruction
  // of TabContainer. In addition, we can avoid redundant layout as a side
  // effect.
  base::OnceClosure LockLayout();

  // TabContainerImpl:
  gfx::Size CalculatePreferredSize() const override;
  void UpdateClosingModeOnRemovedTab(int model_index, bool was_active) override;
  gfx::Rect GetTargetBoundsForClosingTab(Tab* tab,
                                         int former_model_index) const override;
  void EnterTabClosingMode(absl::optional<int> override_width,
                           CloseTabSource source) override;
  bool ShouldTabBeVisible(const Tab* tab) const override;
  void StartInsertTabAnimation(int model_index) override;
  void RemoveTab(int index, bool was_active) override;
  void OnTabCloseAnimationCompleted(Tab* tab) override;
  void CompleteAnimationAndLayout() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void PaintChildren(const views::PaintInfo& paint_info) override;

 private:
  void UpdateLayoutOrientation();

  void OnUnlockLayout();

  base::flat_set<Tab*> closing_tabs_;

  raw_ptr<TabDragContext> drag_context_;

  // A pointer storing the global tab style to be used.
  const raw_ptr<const TabStyle> tab_style_;

  BooleanPrefMember show_vertical_tabs_;
  BooleanPrefMember vertical_tabs_floating_mode_enabled_;
  BooleanPrefMember vertical_tabs_collapsed_;

  bool layout_locked_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_
