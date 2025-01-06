/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_color_mixer.h"

#include "base/containers/fixed_flat_map.h"
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

SkColor IsToolbarColorDark(const ui::ColorProviderKey& key,
                           const ui::ColorMixer& mixer) {
  CHECK(key.custom_theme);
  auto toolbar_color = mixer.GetResultColor(kColorToolbar);
  SkColor custom_toolbar_color;
  if (key.custom_theme->GetColor(ThemeProperties::COLOR_TOOLBAR,
                                 &custom_toolbar_color)) {
    toolbar_color = custom_toolbar_color;
  }

  return color_utils::IsDark(toolbar_color);
}

void AddBraveTabThemeColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderKey& key) {
  auto& mixer = provider->AddMixer();

  if (key.custom_theme) {
    const bool is_toolbar_dark = IsToolbarColorDark(key, mixer);
    mixer[kColorBraveSplitViewTileBackgroundHorizontal] = {
        is_toolbar_dark ? nala::kColorPrimitiveNeutral10
                        : nala::kColorPrimitiveNeutral80};
    mixer[kColorBraveSplitViewTileBackgroundVertical] = {
        kColorBraveSplitViewTileBackgroundHorizontal};
    mixer[kColorBraveSplitViewTileDivider] = {
        is_toolbar_dark ? nala::kColorPrimitiveNeutral20
                        : nala::kColorPrimitiveNeutral90};
  } else {
    mixer[kColorBraveSplitViewTileBackgroundHorizontal] = {
        nala::kColorDesktopbrowserTabbarSplitViewBackgroundHorizontal};
    mixer[kColorBraveSplitViewTileBackgroundVertical] = {
        nala::kColorDesktopbrowserTabbarSplitViewBackgroundVertical};
    mixer[kColorBraveSplitViewTileDivider] = {
        nala::kColorDesktopbrowserTabbarSplitViewDivider};
  }

  mixer[kColorBraveVerticalTabActiveBackground] = {
      nala::kColorDesktopbrowserTabbarActiveTabVertical};
  mixer[kColorBraveVerticalTabHoveredBackground] = {
      nala::kColorDesktopbrowserTabbarHoverTabVertical};
  mixer[kColorBraveVerticalTabInactiveBackground] = {kColorToolbar};
  mixer[kColorBraveVerticalTabSeparator] = {kColorToolbarContentAreaSeparator};
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
      nala::kColorIconInteractive};
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
  mixer[kColorBraveSplitViewInactiveWebViewBorder] = {kColorToolbar};
}

void AddBraveTabPrivateThemeColorMixer(ui::ColorProvider* provider,
                                       const ui::ColorProviderKey& key) {
  auto& mixer = provider->AddMixer();
  mixer[kColorBraveVerticalTabActiveBackground] = {
      nala::kColorPrimitivePrivateWindow30};
  mixer[kColorBraveVerticalTabInactiveBackground] = {
      mixer.GetResultColor(kColorToolbar)};
  mixer[kColorBraveVerticalTabHoveredBackground] = {
      ui::AlphaBlend(kColorBraveVerticalTabActiveBackground,
                     kColorBraveVerticalTabInactiveBackground,
                     /* 40% opacity */ 0.4 * SK_AlphaOPAQUE)};
  mixer[kColorBraveSplitViewTileBackgroundHorizontal] = {
      nala::kColorPrimitivePrivateWindow10};
  mixer[kColorBraveSplitViewTileBackgroundVertical] = {
      nala::kColorPrimitivePrivateWindow5};
  mixer[kColorBraveSplitViewTileDivider] = {
      nala::kColorPrimitivePrivateWindow20};
}

void AddBraveTabTorThemeColorMixer(ui::ColorProvider* provider,
                                   const ui::ColorProviderKey& key) {
  auto& mixer = provider->AddMixer();
  mixer[kColorBraveVerticalTabActiveBackground] = {
      nala::kColorPrimitiveTorWindow30};
  mixer[kColorBraveVerticalTabInactiveBackground] = {
      mixer.GetResultColor(kColorToolbar)};
  mixer[kColorBraveVerticalTabHoveredBackground] = {
      ui::AlphaBlend(kColorBraveVerticalTabActiveBackground,
                     kColorBraveVerticalTabInactiveBackground,
                     /* 40% opacity */ 0.4 * SK_AlphaOPAQUE)};
  mixer[kColorBraveSplitViewTileBackgroundHorizontal] = {
      nala::kColorPrimitiveTorWindow10};
  mixer[kColorBraveSplitViewTileBackgroundVertical] = {
      nala::kColorPrimitiveTorWindow5};
  mixer[kColorBraveSplitViewTileDivider] = {nala::kColorPrimitiveTorWindow20};
}

}  // namespace tabs
