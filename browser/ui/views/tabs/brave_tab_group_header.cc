/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"

#include <optional>

#include "base/check.h"
#include "base/i18n/rtl.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/tab_group_style.h"
#include "chrome/browser/ui/views/tabs/tab_group_underline.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/background.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/label.h"
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

views::BubbleBorder::Arrow BraveTabGroupHeader::GetAnchorPosition() const {
  const BrowserWindowInterface* browser =
      tab_slot_controller_->GetBrowserWindowInterface();
  if (!tabs::utils::ShouldShowBraveVerticalTabs(browser)) {
    return TabGroupHeader::GetAnchorPosition();
  }

  const auto arrow = tabs::utils::IsVerticalTabOnRight(browser)
                         ? views::BubbleBorder::Arrow::RIGHT_TOP
                         : views::BubbleBorder::Arrow::LEFT_TOP;
  // BubbleDialogDelegate mirrors arrows for RTL layouts. Vertical tab side
  // preferences are physical, so compensate here to keep the bubble opening
  // toward the web contents.
  return base::i18n::IsRTL() ? views::BubbleBorder::horizontal_mirror(arrow)
                             : arrow;
}

views::BubbleBorder::Arrow BraveTabGroupHeader::GetEditorBubbleArrow(
    views::BubbleBorder::Arrow chromium_arrow) const {
  auto arrow = GetAnchorPosition();
  // Preserve Chromium's near-bottom fallback on Wayland, where automatic
  // offscreen adjustment is unavailable.
  if (chromium_arrow == views::BubbleBorder::Arrow::BOTTOM_LEFT ||
      chromium_arrow == views::BubbleBorder::Arrow::BOTTOM_RIGHT) {
    arrow = views::BubbleBorder::vertical_mirror(arrow);
  }
  return arrow;
}

void BraveTabGroupHeader::VisualsChanged() {
  TabGroupHeader::VisualsChanged();

  if (!tabs::HorizontalTabsUpdateEnabled() && !ShouldShowVerticalTabs()) {
    return;
  }

  title_->SetEnabledColor(GetGroupColor());
  title_->SetSubpixelRenderingEnabled(false);

  if (!ShouldShowVerticalTabs()) {
    title_->SetLineHeight(tabs::kTabGroupLineHeight);
  }

  auto font_list = title_->font_list();
  title_->SetFontList(font_list.DeriveWithWeight(gfx::Font::Weight::MEDIUM)
                          .DeriveWithSizeDelta(13 - font_list.GetFontSize()));

  if (auto chip_background_color = GetChipBackgroundColor()) {
    title_chip_->SetBackground(views::CreateRoundedRectBackground(
        *chip_background_color, group_style_->GetChipCornerRadius()));
  } else {
    title_chip_->SetBackground(nullptr);
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
        kLeoProductSyncIcon, SkColorSetA(GetGroupColor(), 0.6 * 255),
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
  return tabs::utils::ShouldShowBraveVerticalTabs(
      tab_slot_controller_->GetBrowserWindowInterface());
}

void BraveTabGroupHeader::LayoutTitleChipForVerticalTabs() {
  auto title_bounds = GetContentsBounds();
  title_bounds.Inset(gfx::Insets(kPaddingForGroup));
  title_chip_->SetBoundsRect(title_bounds);

  // |title_| is a child view of |title_chip_| and there could be |sync_icon_|
  // before |title_|. Expand |title_|'s width to fill the chip, and set its
  // height to the chip height so the label centers text vertically.
  title_->SetBounds(title_->x(), 0, title_bounds.width() - title_->x(),
                    title_bounds.height());
}

SkColor BraveTabGroupHeader::GetGroupColor() const {
  auto group_id = group().value();

  auto model_contains_group = [&]() {
    if (auto* browser = tab_slot_controller_->GetBrowserWindowInterface()) {
      return browser->GetTabStripModel()->group_model()->ContainsTabGroup(
          group_id);
    }
    return false;
  };

  if (!model_contains_group()) {
    // Can happen in unit tests or in tear-down.
    return gfx::kPlaceholderColor;
  }

  return tab_slot_controller_->GetPaintedGroupColor(
      tab_slot_controller_->GetGroupColorId(group_id));
}

std::optional<SkColor> BraveTabGroupHeader::GetChipBackgroundColor() const {
  if (ShouldShowVerticalTabs()) {
    return {};
  }

  auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return {};
  }

  SkColor blend_background = TabStyle::Get()->GetTabBackgroundColor(
      TabStyle::TabSelectionState::kInactive, /*hovered=*/false,
      GetWidget()->ShouldPaintAsActive(), color_provider);

  SkAlpha alpha =
      SkColorGetA(color_provider->GetColor(kColorTabGroupBackgroundAlpha));

  return color_utils::AlphaBlend(GetGroupColor(), blend_background, alpha);
}

TabNestingInfo BraveTabGroupHeader::GetTabNestingInfo() const {
  if (!tree_tab_node().has_value()) {
    return TabNestingInfo{};
  }
  return {
      .tree_height = tab_slot_controller_->GetTreeHeight(*tree_tab_node()),
      .level = tab_slot_controller_->GetTreeTabNode(*tree_tab_node())->level()};
}

BEGIN_METADATA(BraveTabGroupHeader)
END_METADATA
