/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"

#include <optional>

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/tab_group_style.h"
#include "chrome/browser/ui/views/tabs/tab_group_underline.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
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

void BraveTabGroupHeader::VisualsChanged() {
  TabGroupHeader::VisualsChanged();

  if (!tabs::features::HorizontalTabsUpdateEnabled() &&
      !ShouldShowVerticalTabs()) {
    return;
  }

  title_->SetEnabledColor(GetGroupColor());
  title_->SetSubpixelRenderingEnabled(false);

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
  }
}

int BraveTabGroupHeader::GetDesiredWidth() const {
  if (!tabs::features::HorizontalTabsUpdateEnabled() ||
      ShouldShowVerticalTabs()) {
    return TabGroupHeader::GetDesiredWidth();
  }
  return brave_tabs::kHorizontalTabInset * 2 + title_chip_->width();
}

void BraveTabGroupHeader::Layout() {
  TabGroupHeader::Layout();
  if (ShouldShowVerticalTabs()) {
    LayoutTitleChipForVerticalTabs();
  }
}

bool BraveTabGroupHeader::ShouldShowVerticalTabs() const {
  return tabs::utils::ShouldShowVerticalTabs(
      tab_slot_controller_->GetBrowser());
}

void BraveTabGroupHeader::LayoutTitleChipForVerticalTabs() {
  auto title_bounds = GetContentsBounds();
  title_bounds.Inset(gfx::Insets(kPaddingForGroup));
  title_chip_->SetBoundsRect(title_bounds);

  // |title_| is a child view of |title_chip_| and there could be |sync_icon_|
  // before |title_|. So expand |title_|'s width considering that.
  title_->SetSize({title_bounds.width() - title_->x(), title_->height()});
}

SkColor BraveTabGroupHeader::GetGroupColor() const {
  auto group_id = group().value();

  if (!tab_slot_controller_->GetBrowser()
           ->tab_strip_model()
           ->group_model()
           ->ContainsTabGroup(group_id)) {
    // Can happen in tear-down.
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
      GetWidget()->ShouldPaintAsActive(), *color_provider);

  SkAlpha alpha =
      SkColorGetA(color_provider->GetColor(kColorTabGroupBackgroundAlpha));

  return color_utils::AlphaBlend(GetGroupColor(), blend_background, alpha);
}

BEGIN_METADATA(BraveTabGroupHeader, TabGroupHeader)
END_METADATA
