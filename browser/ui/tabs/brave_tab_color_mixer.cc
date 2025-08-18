/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_color_mixer.h"

#include "base/containers/fixed_flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/logging.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_recipe.h"
#include "ui/color/color_transform.h"
#include "ui/gfx/color_utils.h"

namespace tabs {

namespace {

SkColor GetActiveVerticalTabBackgroundColor(const ui::ColorProviderKey& key,
                                            SkColor input,
                                            const ui::ColorMixer& mixer) {
  const auto default_color =
      mixer.GetResultColor(nala::kColorDesktopbrowserTabbarActiveTabVertical);
  if (!key.user_color.has_value()) {
    return default_color;
  }

  // Extract Hue of tab foreground color and apply Saturation and Lightness for
  // vertical tabs.
  color_utils::HSL hsl;
  color_utils::SkColorToHSL(*key.user_color, &hsl);

  hsl.s = 0.6;    // A little more saturation as default color is grayish.
  hsl.l = 0.485;  // A little bit darker

  return color_utils::HSLShift(default_color, hsl);
}

}  // namespace

void AddBraveTabThemeColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderKey& key) {
  auto& mixer = provider->AddMixer();

  if (key.custom_theme) {
    mixer[kColorBraveSplitViewTileBackgroundHorizontal] = {
        SkColorSetARGB(0x1A, 0x00, 0x00, 0x00)};
    mixer[kColorBraveSplitViewTileBackgroundVertical] = {
        kColorBraveSplitViewTileBackgroundHorizontal};
    mixer[kColorBraveSplitViewTileBackgroundBorder] = {
        SkColorSetARGB(0x34, 0xFF, 0xFF, 0xFF)};
    mixer[kColorBraveSplitViewTileDivider] = {kColorTabDividerFrameActive};
    mixer[kColorBraveVerticalTabActiveBackground] = {
        kColorTabBackgroundInactiveFrameActive};
    mixer[kColorBraveVerticalTabHoveredBackground] = {
        ui::AlphaBlend(kColorBraveVerticalTabActiveBackground,
                       kColorBraveVerticalTabInactiveBackground,
                       /* 40% opacity */ 0.4 * SK_AlphaOPAQUE)};
  } else {
    mixer[kColorBraveSplitViewTileBackgroundHorizontal] = {
        nala::kColorDesktopbrowserTabbarSplitViewBackgroundHorizontal};
    mixer[kColorBraveSplitViewTileBackgroundVertical] = {
        nala::kColorDesktopbrowserTabbarSplitViewBackgroundVertical};
    mixer[kColorBraveSplitViewTileBackgroundBorder] = {SK_ColorTRANSPARENT};
    mixer[kColorBraveSplitViewTileDivider] = {
        nala::kColorDesktopbrowserTabbarSplitViewDivider};
    mixer[kColorBraveVerticalTabActiveBackground] = {
        base::BindRepeating(&GetActiveVerticalTabBackgroundColor, key)};
    mixer[kColorBraveVerticalTabHoveredBackground] = {
        nala::kColorDesktopbrowserTabbarHoverTabVertical};
  }

  mixer[kColorBraveVerticalTabInactiveBackground] = {kColorToolbar};
  mixer[kColorBraveVerticalTabSeparator] = {
      nala::kColorDesktopbrowserToolbarButtonOutline};
  mixer[kColorBraveVerticalTabNTBIconColor] = {
      kColorTabForegroundInactiveFrameActive};
  mixer[kColorBraveVerticalTabNTBTextColor] = {
      kColorTabForegroundInactiveFrameActive};
  mixer[kColorBraveVerticalTabNTBShortcutTextColor] = {
      kColorTabForegroundActiveFrameActive};
  mixer[kColorBraveSplitViewMenuItemIcon] = {nala::kColorIconDefault};
  mixer[kColorBraveSplitViewUrl] = {nala::kColorTextTertiary};
  mixer[kColorBraveSplitViewMenuButtonBorder] = {nala::kColorDividerSubtle};
  mixer[kColorBraveSplitViewActiveWebViewBorder] = {
      nala::kColorPrimitiveBrandsRorange1};
  mixer[kColorBraveSplitViewMenuButtonBackground] = {
      nala::kColorContainerBackground};
  mixer[kColorBraveSplitViewMenuButtonIcon] = {nala::kColorIconInteractive};

  mixer[kColorBraveSharedPinnedTabDummyViewThumbnailBorder] = {
      nala::kColorDividerSubtle};
  mixer[kColorBraveSharedPinnedTabDummyViewDescription] = {
      nala::kColorTextSecondary};
  mixer[kColorBraveSharedPinnedTabDummyViewTitle] = {nala::kColorTextPrimary};
  mixer[kColorBraveSharedPinnedTabDummyViewBackground] = {
      nala::kColorContainerBackground};
  mixer[kColorBraveSplitViewInactiveWebViewBorder] = {
      nala::kColorDesktopbrowserToolbarButtonOutline};
}

void AddBraveTabPrivateThemeColorMixer(ui::ColorProvider* provider,
                                       const ui::ColorProviderKey& key) {
  auto& mixer = provider->AddMixer();
  mixer[kColorBraveVerticalTabActiveBackground] = {
      nala::kColorPrimitivePrivateWindow30};
  mixer[kColorBraveVerticalTabInactiveBackground] = {
      mixer.GetResultColor(kColorToolbar)};
  mixer[kColorBraveVerticalTabHoveredBackground] = {
      nala::kColorPrimitivePrivateWindow15};
  mixer[kColorBraveSplitViewTileBackgroundHorizontal] = {
      nala::kColorPrimitivePrivateWindow10};
  mixer[kColorBraveSplitViewTileBackgroundVertical] = {
      nala::kColorPrimitivePrivateWindow5};
  mixer[kColorBraveSplitViewTileDivider] = {
      nala::kColorPrimitivePrivateWindow20};
  mixer[kColorBraveSplitViewTileBackgroundBorder] = {SK_ColorTRANSPARENT};
}

void AddBraveTabTorThemeColorMixer(ui::ColorProvider* provider,
                                   const ui::ColorProviderKey& key) {
  auto& mixer = provider->AddMixer();
  mixer[kColorBraveVerticalTabActiveBackground] = {
      nala::kColorPrimitiveTorWindow30};
  mixer[kColorBraveVerticalTabInactiveBackground] = {
      mixer.GetResultColor(kColorToolbar)};
  mixer[kColorBraveVerticalTabHoveredBackground] = {
      nala::kColorPrimitiveTorWindow15};
  mixer[kColorBraveSplitViewTileBackgroundHorizontal] = {
      nala::kColorPrimitiveTorWindow10};
  mixer[kColorBraveSplitViewTileBackgroundVertical] = {
      nala::kColorPrimitiveTorWindow5};
  mixer[kColorBraveSplitViewTileDivider] = {nala::kColorPrimitiveTorWindow20};
  mixer[kColorBraveSplitViewTileBackgroundBorder] = {SK_ColorTRANSPARENT};
}

}  // namespace tabs
