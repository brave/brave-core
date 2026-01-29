/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_browser_app_menu_button.h"

#include "base/check_deref.h"
#include "brave/browser/ui/page_info/features.h"
#include "brave/common/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/toolbar/app_menu_icon_controller.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "components/prefs/pref_service.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/border.h"

namespace {

constexpr int kBrandedIconSize = 24;
constexpr int kBrandedBorderThickness = 1;
constexpr int kBrandedLabelExtraSpacing = 3;

bool ShowBrandedIcon() {
  return page_info::features::IsShowBraveShieldsInPageInfoEnabled();
}

}  // namespace

BraveBrowserAppMenuButton::BraveBrowserAppMenuButton(ToolbarView* toolbar_view)
    : BrowserAppMenuButton(toolbar_view),
      toolbar_view_(CHECK_DEREF(toolbar_view)) {
  if (ShowBrandedIcon()) {
    // Increase spacing between label and icon since the branded icon has no
    // built-in left padding.
    SetImageLabelSpacing(GetImageLabelSpacing() + kBrandedLabelExtraSpacing);

    // Listen for pref changes to update the icon.
    pref_change_registrar_.Init(
        toolbar_view_->browser()->profile()->GetPrefs());
    pref_change_registrar_.Add(
        kBraveSubtleAppMenuLogo,
        base::BindRepeating(&BraveBrowserAppMenuButton::UpdateIcon,
                            base::Unretained(this)));
  }
}

BraveBrowserAppMenuButton::~BraveBrowserAppMenuButton() = default;

std::optional<SkColor> BraveBrowserAppMenuButton::GetHighlightTextColor()
    const {
  return std::nullopt;
}

std::optional<SkColor> BraveBrowserAppMenuButton::GetColorForSeverity() const {
  const auto* const color_provider = GetColorProvider();
  switch (type_and_severity_.severity) {
    case AppMenuIconController::Severity::kNone:
      return std::nullopt;
    case AppMenuIconController::Severity::kLow:
      return color_provider->GetColor(kColorAppMenuHighlightSeverityLow);
    case AppMenuIconController::Severity::kMedium:
      return color_provider->GetColor(kColorAppMenuHighlightSeverityMedium);
    case AppMenuIconController::Severity::kHigh:
      return color_provider->GetColor(kColorAppMenuHighlightSeverityHigh);
  }
}

std::optional<SkColor> BraveBrowserAppMenuButton::GetHighlightColor() const {
  return GetColorForSeverity();
}

SkColor BraveBrowserAppMenuButton::GetForegroundColor(ButtonState state) const {
  std::optional<SkColor> color = GetColorForSeverity();
  return color.has_value() ? color.value()
                           : BrowserAppMenuButton::GetForegroundColor(state);
}

bool BraveBrowserAppMenuButton::ShouldPaintBorder() const {
  return true;
}

bool BraveBrowserAppMenuButton::ShouldBlendHighlightColor() const {
  return true;
}

void BraveBrowserAppMenuButton::UpdateLayoutInsets() {
  // Same Insets for W/O Label.
  SetLayoutInsets(::GetLayoutInsets(TOOLBAR_BUTTON));
  if (ShowBrandedIcon()) {
    // The branded app menu icon is larger than a standard icon. Use insets
    // delta to compensate for the difference.
    SetLayoutInsetDelta(gfx::Insets::TLBR(-1, 2, -1, 4));
  }
}

void BraveBrowserAppMenuButton::UpdateIcon() {
  if (!ShowBrandedIcon()) {
    BrowserAppMenuButton::UpdateIcon();
    return;
  }

  auto* prefs = toolbar_view_->browser()->profile()->GetPrefs();
  auto& icon = prefs->GetBoolean(kBraveSubtleAppMenuLogo) ? kAppMenuOutlineIcon
                                                          : kAppMenuColorIcon;
  for (auto state : kButtonStates) {
    SetImageModel(
        state, ui::ImageModel::FromVectorIcon(icon, GetForegroundColor(state),
                                              kBrandedIconSize));
  }
}

void BraveBrowserAppMenuButton::UpdateColorsAndInsets() {
  BrowserAppMenuButton::UpdateColorsAndInsets();

  if (ShowBrandedIcon() && !IsLabelPresentAndVisible()) {
    // Draw a subtle border around the branded app menu button.
    auto border = views::CreateRoundedRectBorder(
        kBrandedBorderThickness, GetRoundedCornerRadius(),
        GetColorProvider()->GetColor(kColorToolbarButtonBorder));
    gfx::Insets extra_insets = GetTargetInsets() - border->GetInsets();
    SetBorder(views::CreatePaddedBorder(std::move(border), extra_insets));
  }
}

BEGIN_METADATA(BraveBrowserAppMenuButton)
END_METADATA
