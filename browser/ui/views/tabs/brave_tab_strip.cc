/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip.h"

#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/views/tabs/brave_tab_drag_context.h"
#include "brave/browser/ui/views/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_strip_controller.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"

BraveTabStrip::BraveTabStrip(std::unique_ptr<TabStripController> controller)
    : TabStrip(std::move(controller)),
      brave_drag_context_(std::make_unique<BraveTabDragContext>(
          this,
          reinterpret_cast<TabDragContext*>(drag_context_.get()))) {}

BraveTabStrip::~BraveTabStrip() = default;

bool BraveTabStrip::ShouldDrawStrokes() const {
  if (!TabStrip::ShouldDrawStrokes())
    return false;

  // Use a little bit lower minimum contrast ratio as our ratio is 1.27979
  // between default tab background and frame color of light theme.
  // With upstream's 1.3f minimum ratio, strokes are drawn and it causes weird
  // border lines in the tab group.
  // Set 1.2797f as a minimum ratio to prevent drawing stroke.
  // We don't need the stroke for our default light theme.
  // NOTE: We don't need to check features::kTabOutlinesInLowContrastThemes
  // enabled state. Althought TabStrip::ShouldDrawStrokes() has related code,
  // that feature is already expired since cr82. See
  // chrome/browser/flag-metadata.json.
  const SkColor background_color = GetTabBackgroundColor(
      TabActive::kActive, BrowserFrameActiveState::kActive);
  const SkColor frame_color =
      controller_->GetFrameColor(BrowserFrameActiveState::kActive);
  const float contrast_ratio =
      color_utils::GetContrastRatio(background_color, frame_color);
  return contrast_ratio < kBraveMinimumContrastRatioForOutlines;
}

void BraveTabStrip::UpdateHoverCard(Tab* tab, HoverCardUpdateType update_type) {
  if (brave_tabs::AreTooltipsEnabled(controller_->GetProfile()->GetPrefs()))
    return;
  TabStrip::UpdateHoverCard(tab, update_type);
}

SkColor BraveTabStrip::GetTabSeparatorColor() const {
  if (tabs::features::ShouldShowVerticalTabs())
    return SK_ColorTRANSPARENT;

  Profile* profile = controller()->GetProfile();
  if (!brave::IsRegularProfile(profile)) {
    if (profile->IsTor())
      return SkColorSetRGB(0x5A, 0x53, 0x66);

    // For incognito/guest window.
    return SkColorSetRGB(0x3F, 0x32, 0x56);
  }

  // If custom theme is used, follow upstream separator color.
  auto* theme_service = ThemeServiceFactory::GetForProfile(profile);
  if (theme_service->GetThemeSupplier())
    return TabStrip::GetTabSeparatorColor();

  bool dark_mode = dark_mode::GetActiveBraveDarkModeType() ==
                   dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  return dark_mode ? SkColorSetRGB(0x39, 0x38, 0x38)
                   : SkColorSetRGB(0xBE, 0xBF, 0xBF);
}

TabDragContext* BraveTabStrip::GetDragContext() {
  return brave_drag_context_.get();
}

BEGIN_METADATA(BraveTabStrip, TabStrip)
END_METADATA
