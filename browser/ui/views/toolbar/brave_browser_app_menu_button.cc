/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_browser_app_menu_button.h"

#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/toolbar/app_menu_icon_controller.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"

std::optional<SkColor> BraveBrowserAppMenuButton::GetHighlightTextColor()
    const {
  return std::nullopt;
}

std::optional<SkColor> BraveBrowserAppMenuButton::GetColorForSeverity() const {
  const auto* const color_provider = GetColorProvider();
  switch (type_and_severity_.severity) {
    case AppMenuIconController::Severity::NONE:
      return std::nullopt;
    case AppMenuIconController::Severity::LOW:
      return color_provider->GetColor(kColorAppMenuHighlightSeverityLow);
    case AppMenuIconController::Severity::MEDIUM:
      return color_provider->GetColor(kColorAppMenuHighlightSeverityMedium);
    case AppMenuIconController::Severity::HIGH:
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

// Same Insest for W/O Label.
void BraveBrowserAppMenuButton::UpdateLayoutInsets() {
  SetLayoutInsets(::GetLayoutInsets(TOOLBAR_BUTTON));
}

BEGIN_METADATA(BraveBrowserAppMenuButton)
END_METADATA
