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

namespace {

SkColor GetGroupBackgroundColorForVerticalTabs(
    const tab_groups::TabGroupId& group_id,
    TabSlotController* controller) {
  DCHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
      << "This should be called only when the flag is on.";

  if (!controller->GetBrowser()
           ->tab_strip_model()
           ->group_model()
           ->ContainsTabGroup(group_id)) {
    // Can happen in tear-down.
    return gfx::kPlaceholderColor;
  }

  return controller->GetPaintedGroupColor(
      controller->GetGroupColorId(group_id));
}

}  // namespace

BraveTabGroupHeader::~BraveTabGroupHeader() = default;

void BraveTabGroupHeader::VisualsChanged() {
  TabGroupHeader::VisualsChanged();
  if (!ShouldShowVerticalTabs())
    return;

  title_->SetEnabledColor(GetGroupBackgroundColorForVerticalTabs(
      group().value(), base::to_address(tab_slot_controller_)));
  title_->SetFontList(
      title_->font_list().DeriveWithWeight(gfx::Font::Weight::MEDIUM));

  // We don't draw background for vertical tabs.
  title_chip_->SetBackground(nullptr);

  LayoutTitleChip();
}

void BraveTabGroupHeader::Layout() {
  TabGroupHeader::Layout();
  if (!ShouldShowVerticalTabs())
    return;

  LayoutTitleChip();
}

bool BraveTabGroupHeader::ShouldShowVerticalTabs() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return false;

  return tabs::features::ShouldShowVerticalTabs(
      tab_slot_controller_->GetBrowser());
}

void BraveTabGroupHeader::LayoutTitleChip() {
  DCHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
      << "This should be called only when the flag is on.";

  auto title_bounds = GetContentsBounds();
  title_bounds.Inset(gfx::Insets(kPaddingForGroup * 2));
  title_chip_->SetBoundsRect(title_bounds);
}

BEGIN_METADATA(BraveTabGroupHeader, TabGroupHeader)
END_METADATA
