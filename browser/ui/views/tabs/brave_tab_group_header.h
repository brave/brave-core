/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_

#include <memory>
#include <optional>

#include "base/functional/callback_helpers.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ui/views/tabs/tab_group_header.h"
#include "chrome/browser/ui/views/tabs/tab_strip_layout_types.h"
#include "ui/views/widget/widget_observer.h"

namespace tab_groups {
class TabGroupId;
}  // namespace tab_groups

class BraveTabGroupHeader : public TabGroupHeader,
                            public views::WidgetObserver {
  METADATA_HEADER(BraveTabGroupHeader, TabGroupHeader)
 public:
  constexpr static int kPaddingForGroup = 4;

  BraveTabGroupHeader(TabSlotController& tab_slot_controller,
                      const tab_groups::TabGroupId& group,
                      const TabGroupStyle& style);
  ~BraveTabGroupHeader() override;

  // TabGroupHeader:
  void AddedToWidget() override;
  void VisualsChanged() override;
  int GetDesiredWidth() const override;
  void Layout(PassKey) override;
  TabNestingInfo GetTabNestingInfo() const override;
  void ShowContextMenuForViewImpl(
      views::View* source,
      const gfx::Point& point,
      ui::mojom::MenuSourceType source_type) override;

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

 private:
  void KeepVerticalTabStripExpandedWhileBubbleIsOpen();

  bool ShouldShowVerticalTabs() const;
  void LayoutTitleChipForVerticalTabs();
  SkColor GetGroupColor() const;
  std::optional<SkColor> GetChipBackgroundColor() const;

  std::unique_ptr<base::ScopedClosureRunner> vertical_tab_state_;
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      bubble_widget_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_
