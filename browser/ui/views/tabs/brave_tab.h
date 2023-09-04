// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_

#include <memory>
#include <string>

#include "chrome/browser/ui/views/tabs/tab.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/point.h"

class BraveTab : public Tab {
 public:
  explicit BraveTab(TabSlotController* controller);
  BraveTab(const BraveTab&) = delete;
  BraveTab& operator=(const BraveTab&) = delete;
  ~BraveTab() override;

  // Tab:
  std::u16string GetTooltipText(const gfx::Point& p) const override;

  // Overridden because we moved alert button to left side in the tab whereas
  // upstream put it on right side. Need to consider this change for calculating
  // largest selectable region.
  int GetWidthOfLargestSelectableRegion() const override;

  void ActiveStateChanged() override;

  absl::optional<SkColor> GetGroupColor() const override;

  void UpdateIconVisibility() override;
  bool ShouldRenderAsNormalTab() const override;
  void Layout() override;
  void ReorderChildLayers(ui::Layer* parent_layer) override;
  void MaybeAdjustLeftForPinnedTab(gfx::Rect* bounds,
                                   int visual_width) const override;

  void ViewHierarchyChanged(
      const views::ViewHierarchyChangedDetails& details) override;
  void OnLayerBoundsChanged(const gfx::Rect& old_bounds,
                            ui::PropertyChangeReason reason) override;
  gfx::Insets GetInsets() const override;

 private:
  friend class BraveTabTest;

  bool IsAtMinWidthForVerticalTabStrip() const;

  void UpdateShadowForActiveTab();
  std::unique_ptr<ui::Layer> CreateShadowLayer();
  void LayoutShadowLayer();

  // TODO(sko) This method could be hopefully replaced with
  // views::View::AddLayerRegion in the latest version. ReorederChildLayers()
  // override could be removed together.
  void AddLayerToBelowThis();

  std::unique_ptr<ui::Layer> shadow_layer_;
  gfx::FontList normal_font_;
  gfx::FontList active_tab_font_;

  static constexpr int kExtraLeftPadding = 4;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_
