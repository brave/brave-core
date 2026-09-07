/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_color_mixer.h"

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_recipe.h"
#include "ui/color/color_transform.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/darker_theme/darker_theme_color_transform_factory.h"
#include "brave/browser/ui/darker_theme/features.h"
#endif  // defined(TOOLKIT_VIEWS)

namespace tabs {

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
        nala::kColorDesktopbrowserTabbarActiveTabVertical};
    mixer[kColorTabBackgroundInactiveHoverFrameActive] = {
        nala::kColorDesktopbrowserTabbarHoverTabHorizontal};
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
  mixer[kColorBraveSplitViewActiveWebViewBorder] = {nala::kColorPrimary50};
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

  auto& postprocessing_mixer = provider->AddPostprocessingMixer();
  auto apply_opacity_for_inactive_tab_foreground =
      base::BindRepeating([](SkColor input, const ui::ColorMixer& mixer) {
        return SkColorSetA(input,
                           0xFF * 0.7);  // 70% opacity of the input
      });

  // Note that this opacity adjustment will be overriden when darker theme
  // is applied.
  postprocessing_mixer[kColorTabForegroundInactiveFrameActive] =
      ui::ColorTransform(apply_opacity_for_inactive_tab_foreground);
  postprocessing_mixer[kColorTabForegroundInactiveFrameInactive] =
      ui::ColorTransform(apply_opacity_for_inactive_tab_foreground);

#if defined(TOOLKIT_VIEWS)
  if (!base::FeatureList::IsEnabled(
          darker_theme::features::kBraveDarkerTheme) ||
      key.custom_theme ||
      key.scheme_variant != ui::ColorProviderKey::SchemeVariant::kDarker) {
    return;
  }

  // Tab background
  // : active/inactive tab X active/inactive frame
  postprocessing_mixer[kColorTabBackgroundActiveFrameActive] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral20);
  postprocessing_mixer[kColorTabBackgroundActiveFrameInactive] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral20);
  postprocessing_mixer[kColorTabBackgroundInactiveFrameActive] = {
      ui::kColorFrameActive};
  postprocessing_mixer[kColorTabBackgroundInactiveFrameInactive] = {
      ui::kColorFrameInactive};

  // Tab foreground - such as title text
  // : active/inactive tab X active/inactive frame
  postprocessing_mixer[kColorTabForegroundActiveFrameActive] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral90);
  postprocessing_mixer[kColorTabForegroundActiveFrameInactive] = {
      kColorTabForegroundActiveFrameActive};
  postprocessing_mixer[kColorTabForegroundInactiveFrameActive] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral60);
  postprocessing_mixer[kColorTabForegroundInactiveFrameInactive] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral60);

  // Tab hovered background
  postprocessing_mixer[kColorTabBackgroundInactiveHoverFrameActive] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral10);

  // Split view tile background - horizontal/vertical
  postprocessing_mixer[kColorBraveSplitViewTileBackgroundHorizontal] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral5);
  postprocessing_mixer[kColorBraveSplitViewTileBackgroundVertical] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral0);

  // NewTabButton
  postprocessing_mixer[kColorNewTabButtonForegroundFrameActive] = {
      kColorTabForegroundActiveFrameActive};
  postprocessing_mixer[kColorNewTabButtonForegroundFrameInactive] = {
      kColorTabForegroundActiveFrameInactive};
  postprocessing_mixer[kColorNewTabButtonBackgroundFrameActive] = {
      ui::kColorFrameActive};
  postprocessing_mixer[kColorNewTabButtonBackgroundFrameInactive] = {
      ui::kColorFrameInactive};

  // Tab-strip control / combo-button icons (tab search, horizontal +,
  // workspaces, scroll chevrons, vertical-tab toolbar search). Chromium maps
  // these CR color IDs through regular-mixer ColorId references, which cannot
  // see this postprocessing mixer's overrides of tab-foreground colors — so
  // without an explicit recipe here the icons keep the regular dark-theme
  // (near-white) color. Match toolbar icons instead.
  const SkColor toolbar_icon =
      postprocessing_mixer.GetResultColor(kColorToolbarButtonIcon);
  const SkColor toolbar_icon_inactive =
      postprocessing_mixer.GetResultColor(kColorToolbarButtonIconInactive);
  postprocessing_mixer[kColorNewTabButtonCRForegroundFrameActive] = {
      toolbar_icon};
  postprocessing_mixer[kColorNewTabButtonCRForegroundFrameInactive] = {
      toolbar_icon_inactive};
  postprocessing_mixer[kColorTabSearchButtonCRForegroundFrameActive] = {
      toolbar_icon};
  postprocessing_mixer[kColorTabSearchButtonCRForegroundFrameInactive] = {
      toolbar_icon_inactive};

  // Vertical tabs
  postprocessing_mixer[kColorBraveVerticalTabActiveBackground] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral20);
  postprocessing_mixer[kColorBraveVerticalTabHoveredBackground] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral10);
  postprocessing_mixer[kColorBraveVerticalTabInactiveBackground] = {
      kColorToolbar};
  postprocessing_mixer[kColorBraveVerticalTabSeparator] = {
      nala::kColorPrimitiveNeutral15};

  postprocessing_mixer[kColorBraveVerticalTabNTBIconColor] = {toolbar_icon};
  postprocessing_mixer[kColorBraveVerticalTabNTBTextColor] = {toolbar_icon};
  postprocessing_mixer[kColorBraveVerticalTabNTBShortcutTextColor] = {
      toolbar_icon};
#endif  // defined(TOOLKIT_VIEWS)
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
