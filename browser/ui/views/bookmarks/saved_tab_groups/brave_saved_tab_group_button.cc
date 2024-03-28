/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/bookmarks/saved_tab_groups/brave_saved_tab_group_button.h"

#include <memory>
#include <utility>

#include "chrome/browser/ui/tabs/tab_group_theme.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/highlight_path_generator.h"

namespace tab_groups {

namespace {

constexpr int kBorderRadius = 4;
constexpr int kBorderThickness = 1;
constexpr int kVerticalInset = 4;
constexpr int kHorizontalInset = 8;
constexpr int kButtonSize = 20;

}  // namespace

BraveSavedTabGroupButton::~BraveSavedTabGroupButton() = default;

void BraveSavedTabGroupButton::PaintButtonContents(gfx::Canvas* canvas) {
  // `SavedTabGroupButton` paints a rounded rect "chip" here if the tab group
  // title is empty. In our case, we simply set the background.
}

void BraveSavedTabGroupButton::Initialize() {
  views::InstallRoundRectHighlightPathGenerator(this, gfx::Insets(0),
                                                kBorderRadius);
}

void BraveSavedTabGroupButton::UpdateButtonLayout() {
  auto* cp = GetColorProvider();

  // Note that upstream uses separate color IDs for the button background,
  // button text, and the outline. We simply use the tab group header foreground
  // color with various opacities.
  ui::ColorId text_color = GetTabGroupTabStripColorId(
      tab_group_color_id_, GetWidget()->ShouldPaintAsActive());

  // Use the tab group color for text in all cases (even when the browser is
  // not the active window).
  SetEnabledTextColorIds(text_color);
  SetTextColorId(STATE_DISABLED, text_color);

  SetBackground(views::CreateRoundedRectBackground(
      SkColorSetA(cp->GetColor(text_color), 0.15 * 255), kBorderRadius,
      kBorderThickness));

  std::unique_ptr<views::Border> border;
  if (!local_group_id_.has_value()) {
    border = views::CreateEmptyBorder(kBorderThickness);
  } else {
    border = views::CreateRoundedRectBorder(
        kBorderThickness, kBorderRadius,
        SkColorSetA(cp->GetColor(text_color), 0.6 * 255));
  }

  SetBorder(views::CreatePaddedBorder(
      std::move(border), gfx::Insets::VH(kVerticalInset, kHorizontalInset)));

  if (GetText().empty()) {
    SetPreferredSize(gfx::Size(kButtonSize, kButtonSize));
  } else {
    SetPreferredSize(CalculatePreferredSize());
  }
}

BEGIN_METADATA(BraveSavedTabGroupButton)
END_METADATA

}  // namespace tab_groups
