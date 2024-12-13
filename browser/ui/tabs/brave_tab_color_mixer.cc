/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_color_mixer.h"

#include "base/containers/fixed_flat_map.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_recipe.h"
#include "ui/color/color_transform.h"

namespace tabs {

void AddBraveTabThemeColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderKey& key) {
  auto& mixer = provider->AddMixer();

  mixer[kColorBraveVerticalTabActiveBackground] = {
      kColorTabBackgroundInactiveFrameActive};
  mixer[kColorBraveVerticalTabInactiveBackground] = {kColorToolbar};
  mixer[kColorBraveVerticalTabInactiveHoverBackground] =
      ui::AlphaBlend(kColorBraveVerticalTabActiveBackground,
                     kColorBraveVerticalTabInactiveBackground,
                     /* 40% opacity */ 0.4 * SK_AlphaOPAQUE);
  mixer[kColorBraveVerticalTabSeparator] = {kColorToolbarContentAreaSeparator};
  mixer[kColorBraveVerticalTabNTBIconColor] = {
      kColorTabForegroundInactiveFrameActive};
  mixer[kColorBraveVerticalTabNTBTextColor] = {
      kColorTabForegroundInactiveFrameActive};
  mixer[kColorBraveVerticalTabNTBShortcutTextColor] = {
      kColorTabForegroundActiveFrameActive};

  mixer[kColorBraveSplitViewTileBackgroundHorizontal] = {
      nala::kColorDesktopbrowserTabbarSplitViewBackgroundHorizontal};
  mixer[kColorBraveSplitViewTileBackgroundVertical] = {
      nala::kColorDesktopbrowserTabbarSplitViewBackgroundVertical};
  mixer[kColorBraveSplitViewTileDivider] = {
      nala::kColorDesktopbrowserTabbarSplitViewDivider};
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
      mixer.GetResultColor(kColorTabBackgroundActiveFrameActive)};
  mixer[kColorBraveVerticalTabInactiveBackground] = {
      mixer.GetResultColor(kColorToolbar)};
  mixer[kColorBraveSplitViewTileBackgroundHorizontal] = {
      nala::kColorPrimitivePrivateWindow10};
  mixer[kColorBraveSplitViewTileBackgroundVertical] = {
      kColorBraveSplitViewTileBackgroundHorizontal};
  mixer[kColorBraveSplitViewTileDivider] = {
      nala::kColorPrimitivePrivateWindow20};
}

void AddBraveTabTorThemeColorMixer(ui::ColorProvider* provider,
                                   const ui::ColorProviderKey& key) {
  auto& mixer = provider->AddMixer();
  mixer[kColorBraveVerticalTabActiveBackground] = {
      mixer.GetResultColor(kColorTabBackgroundActiveFrameActive)};
  mixer[kColorBraveVerticalTabInactiveBackground] = {
      mixer.GetResultColor(kColorToolbar)};
  mixer[kColorBraveSplitViewTileBackgroundHorizontal] = {
      nala::kColorPrimitiveTorWindow10};
  mixer[kColorBraveSplitViewTileBackgroundVertical] = {
      kColorBraveSplitViewTileBackgroundHorizontal};
  mixer[kColorBraveSplitViewTileDivider] = {nala::kColorPrimitiveTorWindow20};
}

}  // namespace tabs
