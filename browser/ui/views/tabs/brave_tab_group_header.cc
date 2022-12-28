/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/tab_group_underline.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/background.h"
#include "ui/views/controls/label.h"

// static
SkColor BraveTabGroupHeader::GetDarkerColorForGroup(
    const tab_groups::TabGroupId& group_id,
    TabSlotController* controller,
    bool dark_mode) {
  if (!controller->GetBrowser()
           ->tab_strip_model()
           ->group_model()
           ->ContainsTabGroup(group_id)) {
    // Can happen in tear-down.
    return gfx::kPlaceholderColor;
  }

  return color_utils::HSLShift(
      controller->GetPaintedGroupColor(controller->GetGroupColorId(group_id)),
      {.h = -1 /*unchanged*/,
       .s = 0.5 /*unchanged*/,
       .l = dark_mode ? 0.2 : 0.3 /*darker*/});
}

BraveTabGroupHeader::~BraveTabGroupHeader() = default;

void BraveTabGroupHeader::VisualsChanged() {
  TabGroupHeader::VisualsChanged();
  if (!tabs::features::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return;
  }

  title_->SetEnabledColor(GetDarkerColorForGroup(
      group().value(), base::to_address(tab_slot_controller_),
      GetNativeTheme()->ShouldUseDarkColors()));
  title_->SetFontList(
      title_->font_list().DeriveWithWeight(gfx::Font::Weight::MEDIUM));

  // We don't need to fill |title_chip_| as we have enclosing box drawn by
  // TabGroupUnderline.
  title_chip_->SetBackground(nullptr);

  LayoutTitleChip();
}

void BraveTabGroupHeader::Layout() {
  TabGroupHeader::Layout();
  if (!tabs::features::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return;
  }

  LayoutTitleChip();
}

void BraveTabGroupHeader::LayoutTitleChip() {
  auto title_bounds = GetContentsBounds();
  title_bounds.Inset(gfx::Insets(kPaddingForGroup * 2));
  title_chip_->SetBoundsRect(title_bounds);
}

BEGIN_METADATA(BraveTabGroupHeader, TabGroupHeader)
END_METADATA
