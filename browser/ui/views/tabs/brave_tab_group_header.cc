/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"

#include <algorithm>

#include "base/check.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/tabs/tab_group_style_views.h"
#include "chrome/browser/ui/views/tabs/tab_group_underline.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

BraveTabGroupHeader::~BraveTabGroupHeader() = default;

void BraveTabGroupHeader::AddedToWidget() {
  TabGroupHeader::AddedToWidget();
  if (!ShouldShowVerticalTabs()) {
    return;
  }

  if (editor_bubble_tracker_.is_open()) {
    auto* bubble_delegate = editor_bubble_tracker_.widget()
                                ->widget_delegate()
                                ->AsBubbleDialogDelegate();
    DCHECK(bubble_delegate);
    // Technically, this can happen when tab strip's orientation changes
    // with the editor bubble open. It re-parents the widget which can cause
    // DCHECK failure. We should call SetAnchorView again to reset
    // the anchor widget.
    bubble_delegate->SetAnchorView(this);
  }
}

void BraveTabGroupHeader::VisualsChanged() {
  TabGroupHeader::VisualsChanged();
  upstream_title_chip_width_ = title_chip_->width();

  if (!tabs::HorizontalTabsUpdateEnabled() && !ShouldShowVerticalTabs()) {
    return;
  }

  if (ShouldShowVerticalTabs()) {
    LayoutTitleChipForVerticalTabs();

    // TabGroupHeader::VisualsChanged() sets |title_chip_|'s clip path based on
    // its bounds at that time, which are narrower than the bounds
    // LayoutTitleChipForVerticalTabs() just applied. Just clear clip path as
    // it's not necessary in vertical tab.
    title_chip_->SetClipPath(SkPath());
  }

  if (ShouldShowHeaderIcon()) {
    sync_icon_->SetImage(ui::ImageModel::FromVectorIcon(
        kLeoProductSyncIcon, color_utils::GetColorWithMaxContrast(color_),
        group_style_->GetSyncIconWidth()));
  }
}

int BraveTabGroupHeader::GetDesiredWidth() const {
  if (!tabs::HorizontalTabsUpdateEnabled() || ShouldShowVerticalTabs()) {
    return TabGroupHeader::GetDesiredWidth();
  }
  return tabs::kHorizontalTabInset * 2 + title_chip_->width();
}

void BraveTabGroupHeader::Layout(PassKey) {
  LayoutSuperclass<TabGroupHeader>(this);
  if (ShouldShowVerticalTabs()) {
    LayoutTitleChipForVerticalTabs();
  }
}

bool BraveTabGroupHeader::ShouldShowVerticalTabs() const {
  auto* vtc = VerticalTabController::From(
      tab_slot_controller_->GetBrowserWindowInterface());
  return vtc && vtc->ShouldShowBraveVerticalTabs();
}

void BraveTabGroupHeader::LayoutTitleChipForVerticalTabs() {
  auto available_bounds = GetContentsBounds();
  available_bounds.Inset(gfx::Insets(kPaddingForGroup));

  if (group_title_.empty()) {
    title_chip_->SetBounds(
        available_bounds.x(), available_bounds.y(),
        std::max(upstream_title_chip_width_, available_bounds.height()),
        available_bounds.height());
    return;
  }

  // Upstream caps the title at half a horizontal tab's width. Instead, make
  // |title_chip_| span the entire width of the vertical tab bar, and let
  // |title_| use the remaining space. Space around |title_| (insets,
  // |sync_icon_|, |attention_indicator_|) is kept.
  const int trailing_space = title_chip_->width() - title_->bounds().right();
  const int title_width =
      std::max(0, available_bounds.width() - title_->x() - trailing_space);

  if (attention_indicator_->GetVisible()) {
    attention_indicator_->SetX(attention_indicator_->x() + title_width -
                               title_->width());
  }

  title_chip_->SetBounds(available_bounds.x(), available_bounds.y(),
                         available_bounds.width(), available_bounds.height());

  // Set |title_|'s height to the chip height so the label centers text
  // vertically.
  title_->SetBounds(title_->x(), 0, title_width, available_bounds.height());
}

TabNestingInfo BraveTabGroupHeader::GetTabNestingInfo() const {
  if (!tree_tab_node().has_value()) {
    return TabNestingInfo{};
  }
  return {
      .tree_height = tab_slot_controller_->GetTreeHeight(*tree_tab_node()),
      .level = tab_slot_controller_->GetTreeTabNode(*tree_tab_node())->level()};
}

views::BubbleBorder::Arrow BraveTabGroupHeader::GetAnchorPosition() const {
  if (ShouldShowVerticalTabs()) {
    return views::BubbleBorder::Arrow::LEFT_TOP;
  }

  return TabGroupHeader::GetAnchorPosition();
}

BEGIN_METADATA(BraveTabGroupHeader)
END_METADATA
