/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/color/brave_color_mixer.h"

#include "base/numerics/safe_conversions.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/ui/color/brave_ref_color_mixer.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/color/chrome_color_provider_utils.h"
#include "chrome/browser/ui/color/material_chrome_color_mixer.h"
#include "chrome/browser/ui/color/material_omnibox_color_mixer.h"
#include "chrome/browser/ui/color/material_side_panel_color_mixer.h"
#include "chrome/browser/ui/color/material_tab_strip_color_mixer.h"
#include "ui/base/ui_base_features.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_provider_manager.h"
#include "ui/color/color_recipe.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/browser/ui/color/playlist/playlist_color_mixer.h"
#include "brave/components/playlist/core/common/features.h"
#endif

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/darker_theme/darker_theme_color_transform_factory.h"
#include "brave/browser/ui/darker_theme/features.h"
#endif  // defined(TOOLKIT_VIEWS)

namespace {

// Copied from //chrome/browser/ui/omnibox//omnibox_theme.h
// As below values are not changed for several years, it would be safe to copy.
// Can't include //chrome/browser/ui/omnibox in here due to circular deps.
constexpr float kOmniboxOpacityHovered = 0.10f;
constexpr float kOmniboxOpacitySelected = 0.16f;

SkColor PickColorContrastingToToolbar(const ui::ColorProviderKey& key,
                                      const ui::ColorMixer& mixer,
                                      SkColor color1,
                                      SkColor color2) {
  // Custom theme's toolbar color will be the final color.
  auto toolbar_color = mixer.GetResultColor(kColorToolbar);
  SkColor custom_toolbar_color;
  if (key.custom_theme &&
      key.custom_theme->GetColor(ThemeProperties::COLOR_TOOLBAR,
                                 &custom_toolbar_color)) {
    toolbar_color = custom_toolbar_color;
  }
  return color_utils::PickContrastingColor(color1, color2, toolbar_color);
}

bool HasCustomToolbarColor(const ui::ColorProviderKey& key) {
  SkColor custom_toolbar_color;
  return key.custom_theme &&
         key.custom_theme->GetColor(ThemeProperties::COLOR_TOOLBAR,
                                    &custom_toolbar_color);
}

#if BUILDFLAG(ENABLE_BRAVE_VPN) || BUILDFLAG(ENABLE_SPEEDREADER)
SkColor PickSimilarColorToToolbar(const ui::ColorProviderKey& key,
                                  const ui::ColorMixer& mixer,
                                  SkColor light_theme_color,
                                  SkColor dark_theme_color) {
  auto toolbar_color = mixer.GetResultColor(kColorToolbar);
  SkColor custom_toolbar_color;
  if (key.custom_theme &&
      key.custom_theme->GetColor(ThemeProperties::COLOR_TOOLBAR,
                                 &custom_toolbar_color)) {
    toolbar_color = custom_toolbar_color;
  }

  // Give min constrast color.
  return color_utils::IsDark(toolbar_color) ? dark_theme_color
                                            : light_theme_color;
}
#endif

bool IsHighContrast() {
#if !defined(USE_AURA)
  ui::NativeTheme* native_theme = nullptr;
#else
  // For high contrast, selected rows use inverted colors to stand out more.
  ui::NativeTheme* native_theme = ui::NativeTheme::GetInstanceForNativeUi();
#endif  // !defined(USE_AURA)
  return native_theme && native_theme->preferred_contrast() ==
                             ui::NativeTheme::PreferredContrast::kMore;
}

SkColor BlendIdTowardsMaxContrast(SkColor color, float opacity) {
  return color_utils::BlendTowardMaxContrast(color,
                                             base::ClampRound(opacity * 0xff));
}

// Hover/Select for the Omnibox are based on opacity and the result color. The
// calculation is always the same.
void AddOmniboxHoverSelect(ui::ColorMixer& mixer) {
  mixer[kColorOmniboxResultsBackgroundHovered] = {BlendIdTowardsMaxContrast(
      mixer.GetResultColor(kColorOmniboxResultsBackground),
      kOmniboxOpacityHovered)};
  mixer[kColorOmniboxResultsBackgroundSelected] = {BlendIdTowardsMaxContrast(
      mixer.GetResultColor(kColorOmniboxResultsBackground),
      kOmniboxOpacitySelected)};
}

#if BUILDFLAG(ENABLE_BRAVE_VPN)
void AddBraveVpnColorMixer(ui::ColorProvider* provider,
                           const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();
  if (key.custom_theme) {
    mixer[kColorBraveVpnButtonIconConnected] = {PickSimilarColorToToolbar(
        key, mixer, mixer.GetResultColor(nala::kColorPrimitiveGreen50),
        mixer.GetResultColor(nala::kColorPrimitiveGreen60))};
    mixer[kColorBraveVpnButtonIconDisconnected] = {PickSimilarColorToToolbar(
        key, mixer, mixer.GetResultColor(nala::kColorPrimitiveNeutral30),
        mixer.GetResultColor(nala::kColorPrimitiveNeutral80))};
    mixer[kColorBraveVpnButtonIconError] = {PickSimilarColorToToolbar(
        key, mixer, mixer.GetResultColor(nala::kColorPrimitiveRed60),
        mixer.GetResultColor(nala::kColorPrimitiveRed50))};
    mixer[kColorBraveVpnButtonErrorBackgroundNormal] = {
        PickSimilarColorToToolbar(
            key, mixer, mixer.GetResultColor(nala::kColorPrimitiveRed95),
            mixer.GetResultColor(nala::kColorPrimitiveRed15))};
  } else {
    mixer[kColorBraveVpnButtonIconConnected] = {
        nala::kColorSystemfeedbackSuccessIcon};
    mixer[kColorBraveVpnButtonIconDisconnected] = {nala::kColorIconDefault};
    mixer[kColorBraveVpnButtonIconError] = {
        nala::kColorSystemfeedbackErrorIcon};
    mixer[kColorBraveVpnButtonErrorBackgroundNormal] = {
        nala::kColorSystemfeedbackErrorBackground};
  }

  mixer[kColorBraveVpnButtonBackgroundNormal] = {kColorToolbar};

#if defined(TOOLKIT_VIEWS)
  if (!base::FeatureList::IsEnabled(
          darker_theme::features::kBraveDarkerTheme) ||
      key.scheme_variant != ui::ColorProviderKey::SchemeVariant::kDarker) {
    return;
  }

  auto& postprocessing_mixer = provider->AddPostprocessingMixer();
  postprocessing_mixer[kColorBraveVpnButtonBackgroundNormal] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral5);
#endif  // defined(TOOLKIT_VIEWS)
}
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
void AddBraveSpeedreaderColorMixer(ui::ColorProvider* provider,
                                   const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  if (key.custom_theme) {
    auto mix_custom = [&mixer, &key](ui::ColorId light_id,
                                     ui::ColorId dark_id) -> SkColor {
      return PickSimilarColorToToolbar(key, mixer,
                                       mixer.GetResultColor(light_id),
                                       mixer.GetResultColor(dark_id));
    };

    mixer[kColorSpeedreaderIcon] = {mix_custom(nala::kColorPrimitivePrimary35,
                                               nala::kColorPrimitivePrimary80)};

    mixer[kColorSpeedreaderToolbarForeground] = {mix_custom(
        nala::kColorPrimitiveNeutral30, nala::kColorPrimitiveNeutral80)};

    mixer[kColorSpeedreaderToolbarBorder] = {mix_custom(
        nala::kColorPrimitiveNeutral90, nala::kColorPrimitiveNeutral25)};

    mixer[kColorSpeedreaderToolbarButtonHover] = {mix_custom(
        nala::kColorPrimitiveNeutral95, nala::kColorPrimitiveNeutral10)};
    mixer[kColorSpeedreaderToolbarButtonActive] = {mix_custom(
        nala::kColorPrimitiveNeutral90, nala::kColorPrimitiveNeutral5)};
    mixer[kColorSpeedreaderToolbarButtonActiveText] = {mix_custom(
        nala::kColorPrimitivePrimary35, nala::kColorPrimitivePrimary80)};
    mixer[kColorSpeedreaderToolbarButtonBorder] = {mix_custom(
        nala::kColorPrimitiveNeutral90, nala::kColorPrimitiveNeutral25)};
  } else {
    mixer[kColorSpeedreaderIcon] = {nala::kColorIconInteractive};
    mixer[kColorSpeedreaderToolbarForeground] = {nala::kColorIconDefault};
    mixer[kColorSpeedreaderToolbarBorder] = {
        nala::kColorDesktopbrowserToolbarButtonOutline};

    mixer[kColorSpeedreaderToolbarButtonHover] = {
        nala::kColorDesktopbrowserToolbarButtonHover};
    mixer[kColorSpeedreaderToolbarButtonActive] = {
        nala::kColorDesktopbrowserToolbarButtonActive};
    mixer[kColorSpeedreaderToolbarButtonActiveText] = {
        nala::kColorIconInteractive};
    mixer[kColorSpeedreaderToolbarButtonBorder] = {
        nala::kColorDesktopbrowserToolbarButtonOutline};
  }

  mixer[kColorSpeedreaderToolbarBackground] = {kColorToolbar};
}
#endif

void AddChromeLightThemeColorMixer(ui::ColorProvider* provider,
                                   const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorTabThrobber] = {SkColorSetRGB(0xd7, 0x55, 0x26)};
  mixer[kColorBookmarkBarForeground] = {kColorTabForegroundActiveFrameActive};
  mixer[ui::kColorBadgeBackground] = {nala::kColorSchemesPrimary};
  mixer[ui::kColorBadgeForeground] = {nala::kColorSchemesOnPrimary};
  mixer[kColorForTest] = {kLightColorForTest};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabPageBackground] = {kBraveNewTabBackgroundLight};
  mixer[kColorTabStrokeFrameActive] = {ui::kColorRefNeutral80};
  mixer[kColorTabStrokeFrameInactive] = {kColorTabStrokeFrameActive};
  mixer[kColorToolbarButtonIconInactive] = {
      ui::SetAlpha(kColorToolbarButtonIcon, kBraveDisabledControlAlpha)};

  mixer[kColorTabCloseButtonFocusRingActive] = {
      ui::kColorFocusableBorderFocused};
  mixer[kColorTabCloseButtonFocusRingInactive] = {
      ui::kColorFocusableBorderFocused};
  mixer[kColorTabFocusRingActive] = {ui::kColorFocusableBorderFocused};
  mixer[kColorTabFocusRingInactive] = {ui::kColorFocusableBorderFocused};

  // Upstream uses tab's background color as omnibox chip background color.
  // In our light mode, there is no difference between location bar's bg
  // color and tab's bg color. So, it looks like chip's bg color is transparent.
  // Use frame color as chip background to have different bg color.
  mixer[kColorOmniboxChipBackground] = {ui::kColorFrameActive};
}

void AddChromeDarkThemeColorMixer(ui::ColorProvider* provider,
                                  const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorTabThrobber] = {SkColorSetRGB(0xd7, 0x55, 0x26)};
  mixer[kColorBookmarkBarForeground] = {kColorTabForegroundActiveFrameActive};
  mixer[ui::kColorBadgeBackground] = {nala::kColorSchemesPrimary};
  mixer[ui::kColorBadgeForeground] = {nala::kColorSchemesOnPrimary};
  mixer[kColorForTest] = {kDarkColorForTest};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabPageBackground] = {kBraveNewTabBackgroundDark};
  mixer[kColorTabStrokeFrameActive] = {ui::kColorRefNeutral25};
  mixer[kColorTabStrokeFrameInactive] = {kColorToolbar};
  mixer[kColorToolbarButtonIconInactive] = {
      ui::SetAlpha(kColorToolbarButtonIcon, kBraveDisabledControlAlpha)};
  mixer[kColorToolbarTopSeparatorFrameActive] = {kColorToolbar};
  mixer[kColorToolbarTopSeparatorFrameInactive] = {kColorToolbar};

  mixer[kColorTabCloseButtonFocusRingActive] = {
      ui::kColorFocusableBorderFocused};
  mixer[kColorTabCloseButtonFocusRingInactive] = {
      ui::kColorFocusableBorderFocused};
  mixer[kColorTabFocusRingActive] = {ui::kColorFocusableBorderFocused};
  mixer[kColorTabFocusRingInactive] = {ui::kColorFocusableBorderFocused};
}

void AddChromeColorMixerForAllThemes(ui::ColorProvider* provider,
                                     const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  // Use same ink drop effect for all themes including custome themes.
  // Toolbar button's inkdrop highlight/visible colors depends on toolbar color.
  auto get_toolbar_ink_drop_color = [](float dark_opacity, float light_opacity,
                                       SkColor input,
                                       const ui::ColorMixer& mixer) {
    const float highlight_opacity =
        color_utils::IsDark(mixer.GetResultColor(kColorToolbar))
            ? dark_opacity
            : light_opacity;
    return SkColorSetA(SK_ColorBLACK, 0xFF * highlight_opacity);
  };
  mixer[kColorToolbarInkDropHover] = {
      base::BindRepeating(get_toolbar_ink_drop_color, 0.25f, 0.05f)};
  mixer[kColorToolbarInkDropRipple] = {
      base::BindRepeating(get_toolbar_ink_drop_color, 0.4f, 0.1f)};

  mixer[ui::kColorFrameActive] = {ui::kColorSysHeader};
  // TODO(simonhong): Should we adjust frame color for inactive window?
  mixer[ui::kColorFrameInactive] = {ui::kColorFrameActive};

  if (key.custom_theme) {
    return;
  }

  // We don't show border when omnibox doesn't have focus but still
  // contains in-progress user input.
  mixer[kColorLocationBarBorderOnMismatch] = {SK_ColorTRANSPARENT};
}

void AddBraveColorMixerForAllThemes(ui::ColorProvider* provider,
                                    const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  // Custom theme will use this color. Other themes could have another
  // color by their color mixers.
  // A primitive so the accent stays fixed on custom theme colors.
  mixer[kColorToolbarButtonActivated] = {nala::kColorPrimitiveBlurple60};
  mixer[kColorSidebarButtonPressed] = {kColorToolbarButtonActivated};

  mixer[kColorLocationBarFocusRing] = {nala::kColorPrimitivePrimary50};

  // Search conversion button in omnibox.
  mixer[kColorSearchConversionButtonText] = {nala::kColorPrimary60};
  mixer[kColorSearchConversionButtonBorder] = {nala::kColorPrimary20};
  mixer[kColorSearchConversionButtonBackground] = {nala::kColorPrimary10};
  mixer[kColorSearchConversionButtonBackgroundHovered] = {
      nala::kColorPrimary20};
  mixer[kColorSearchConversionButtonCloseButton] = {
      ui::AlphaBlend({nala::kColorIconInteractive},
                     {kColorSearchConversionButtonBackground}, 0.5 * 0xff)};
  mixer[kColorSearchConversionButtonCloseButtonHovered] = {
      ui::AlphaBlend({nala::kColorIconInteractive},
                     {kColorSearchConversionButtonBackground}, 0.7 * 0xff)};
  mixer[kColorSearchConversionButtonCaratRight] = {
      kColorSearchConversionButtonCloseButton};
  mixer[kColorSearchConversionButtonShadow1] = {
      SkColorSetA(SK_ColorBLACK, 0.05 * 255)};
  mixer[kColorSearchConversionButtonShadow2] = {
      SkColorSetA(SK_ColorBLACK, 0.1 * 255)};
}

}  // namespace

void AddBravifiedChromeThemeColorMixer(ui::ColorProvider* provider,
                                       const ui::ColorProviderKey& key) {
  AddChromeColorMixerForAllThemes(provider, key);

  AddMaterialChromeColorMixer(provider, key);
  AddMaterialOmniboxColorMixer(provider, key);
  AddMaterialTabStripColorMixer(provider, key);
  AddMaterialSidePanelColorMixer(provider, key);

  // TODO(simonhong): Move these overriding to
  // AddChromeColorMixerForAllThemes().
  ui::ColorMixer& mixer = provider->AddMixer();
  mixer[kColorAppMenuHighlightSeverityLow] = {PickColorContrastingToToolbar(
      key, mixer, mixer.GetResultColor(nala::kColorPrimitiveGreen20),
      mixer.GetResultColor(nala::kColorPrimitiveGreen70))};
  mixer[kColorAppMenuHighlightSeverityMedium] = {PickColorContrastingToToolbar(
      key, mixer, mixer.GetResultColor(nala::kColorPrimitiveYellow30),
      mixer.GetResultColor(nala::kColorPrimitiveYellow80))};
  mixer[kColorAppMenuHighlightSeverityHigh] = {PickColorContrastingToToolbar(
      key, mixer, mixer.GetResultColor(nala::kColorPrimitiveRed35),
      mixer.GetResultColor(nala::kColorPrimitiveRed80))};

  if (key.custom_theme) {
    return;
  }

  mixer[kColorToolbar] = {ui::kColorSysBase};
  mixer[kColorDownloadToolbarButtonActive] = {nala::kColorIconInteractive};
  mixer[kColorDownloadToolbarButtonRingBackground] = {SkColorSetA(
      mixer.GetResultColor(kColorDownloadToolbarButtonActive), 0.3 * 255)};
  mixer[kColorToolbarActionItemEngaged] = {nala::kColorIconInteractive};

  // Match the Leo toggle component. Leo uses plain white for the unchecked
  // thumb, which has no semantic token.
  mixer[ui::kColorToggleButtonThumbOff] = {nala::kColorPrimitiveNeutral100};
  mixer[ui::kColorToggleButtonThumbOn] = {nala::kColorSchemesOnPrimary};
  mixer[ui::kColorToggleButtonTrackOff] = {nala::kColorSchemesOutlineVariant};
  mixer[ui::kColorToggleButtonTrackOn] = {nala::kColorSchemesPrimary};

  key.color_mode == ui::ColorProviderKey::ColorMode::kDark
      ? AddChromeDarkThemeColorMixer(provider, key)
      : AddChromeLightThemeColorMixer(provider, key);
}

void AddBraveLightThemeColorMixer(ui::ColorProvider* provider,
                                  const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorForTest] = {kLightColorForTest};

  mixer[kColorBookmarkBarInstructionsText] = {PickColorContrastingToToolbar(
      key, mixer, mixer.GetResultColor(nala::kColorPrimitiveNeutral35),
      mixer.GetResultColor(nala::kColorPrimitiveNeutral100))};
  mixer[kColorMenuItemSubText] = {nala::kColorTextTertiary};
  mixer[kColorSearchConversionBannerTypeBackground] = {nala::kColorBlue10};
  mixer[kColorSearchConversionBannerTypeDescText] = {nala::kColorTextPrimary};
  mixer[kColorSearchConversionBannerTypeBackgroundBorder] = {
      nala::kColorPrimary20};
  mixer[kColorSearchConversionBannerTypeBackgroundBorderHovered] = {
      nala::kColorPrimary30};
  mixer[kColorSearchConversionBannerTypeBackgroundGradientFrom] = {
      ui::SetAlpha(nala::kColorContainerBackground, 104)};
  mixer[kColorSearchConversionBannerTypeBackgroundGradientTo] = {
      nala::kColorPrimary10};

  mixer[kColorDialogDontAskAgainButton] = {nala::kColorTextTertiary};
  mixer[kColorDialogDontAskAgainButtonHovered] = {nala::kColorTextSecondary};
  mixer[kColorSidebarAddBubbleHeaderText] = {nala::kColorTextPrimary};
  mixer[kColorSidebarAddBubbleItemTextBackgroundHovered] = {
      nala::kColorButtonBackground};
  mixer[kColorSidebarAddBubbleItemTextHovered] = {nala::kColorSchemesOnPrimary};
  mixer[kColorSidebarAddBubbleItemTextNormal] = {nala::kColorTextPrimary};
  mixer[kColorSidebarArrowBackgroundHovered] = {kColorToolbarInkDropHover};
  mixer[kColorSidebarSeparator] = {nala::kColorDividerSubtle};

  mixer[kColorSidebarButtonBase] = {kColorToolbarButtonIcon};
  if (!HasCustomToolbarColor(key)) {
    mixer[kColorSidebarButtonPressed] = {kColorToolbarButtonActivated};
  }

  mixer[kColorSidebarArrowDisabled] = {PickColorContrastingToToolbar(
      key, mixer,
      SkColorSetA(mixer.GetResultColor(nala::kColorPrimitiveNeutral35), 0x8A),
      SkColorSetA(mixer.GetResultColor(nala::kColorPrimitiveNeutralVariant70),
                  0x8A))};
  mixer[kColorSidebarArrowNormal] = {kColorSidebarButtonBase};
  mixer[kColorSidebarItemDragIndicator] = {PickColorContrastingToToolbar(
      key, mixer, mixer.GetResultColor(nala::kColorPrimitiveNeutral15),
      mixer.GetResultColor(nala::kColorPrimitiveNeutralVariant80))};
  mixer[kColorWebDiscoveryInfoBarBackground] = {
      nala::kColorContainerBackground};
  mixer[kColorWebDiscoveryInfoBarMessage] = {nala::kColorTextPrimary};
  mixer[kColorWebDiscoveryInfoBarLink] = {nala::kColorTextInteractive};
  mixer[kColorWebDiscoveryInfoBarNoThanks] = {nala::kColorTextTertiary};
  mixer[kColorWebDiscoveryInfoBarClose] = {nala::kColorIconSecondary};

  // Color for download button when all completed and button needs user
  // interaction.
  mixer[kColorBraveDownloadToolbarButtonActive] = {nala::kColorIconInteractive};

  mixer[kColorLocationBarHoveredShadow] = {
      SkColorSetA(SK_ColorBLACK, 0.07 * 255)};

  // Colors for HelpBubble. IDs are defined in
  // chrome/browser/ui/color/chrome_color_id.h
  mixer[kColorFeaturePromoBubbleBackground] = {nala::kColorContainerBackground};
  mixer[kColorFeaturePromoBubbleForeground] = {nala::kColorTextSecondary};
  mixer[kColorFeaturePromoBubbleCloseButtonInkDrop] = {
      kColorToolbarInkDropHover};

  mixer[kColorTabGroupBackgroundAlpha] = {
      SkColorSetA(SK_ColorBLACK, 0.15 * 255)};

  mixer[ui::kColorMenuButtonBackground] = {SK_ColorTRANSPARENT};
}

void AddBraveDarkThemeColorMixer(ui::ColorProvider* provider,
                                 const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorForTest] = {kDarkColorForTest};

  mixer[kColorBookmarkBarInstructionsText] = {PickColorContrastingToToolbar(
      key, mixer, mixer.GetResultColor(nala::kColorPrimitiveNeutral35),
      mixer.GetResultColor(nala::kColorPrimitiveNeutral100))};
  mixer[kColorMenuItemSubText] = {nala::kColorTextTertiary};
  mixer[kColorSearchConversionBannerTypeDescText] = {nala::kColorTextPrimary};
  mixer[kColorSearchConversionBannerTypeBackgroundBorder] = {
      nala::kColorPrimary20};
  mixer[kColorSearchConversionBannerTypeBackgroundBorderHovered] = {
      nala::kColorPrimary30};
  mixer[kColorSearchConversionBannerTypeBackgroundGradientFrom] = {
      ui::SetAlpha(nala::kColorContainerBackground, 104)};
  mixer[kColorSearchConversionBannerTypeBackgroundGradientTo] = {
      nala::kColorPrimary10};
  mixer[kColorDialogDontAskAgainButton] = {nala::kColorTextTertiary};
  mixer[kColorDialogDontAskAgainButtonHovered] = {nala::kColorTextSecondary};
  mixer[kColorSidebarAddBubbleHeaderText] = {nala::kColorTextPrimary};
  mixer[kColorSidebarAddBubbleItemTextBackgroundHovered] = {
      nala::kColorButtonBackground};
  mixer[kColorSidebarAddBubbleItemTextHovered] = {nala::kColorSchemesOnPrimary};
  mixer[kColorSidebarAddBubbleItemTextNormal] = {nala::kColorTextPrimary};
  mixer[kColorSidebarArrowBackgroundHovered] = {kColorToolbarInkDropHover};
  mixer[kColorSidebarSeparator] = {nala::kColorDividerSubtle};

  mixer[kColorSidebarButtonBase] = {kColorToolbarButtonIcon};
  if (!HasCustomToolbarColor(key)) {
    mixer[kColorSidebarButtonPressed] = {kColorToolbarButtonActivated};
  }
  mixer[kColorSidebarArrowDisabled] = {PickColorContrastingToToolbar(
      key, mixer,
      SkColorSetA(mixer.GetResultColor(nala::kColorPrimitiveNeutral35), 0x8A),
      SkColorSetA(mixer.GetResultColor(nala::kColorPrimitiveNeutralVariant70),
                  0x8A))};
  mixer[kColorSidebarArrowNormal] = {kColorSidebarButtonBase};
  mixer[kColorSidebarItemDragIndicator] = {PickColorContrastingToToolbar(
      key, mixer, mixer.GetResultColor(nala::kColorPrimitiveNeutral15),
      mixer.GetResultColor(nala::kColorPrimitiveNeutralVariant80))};
  mixer[kColorWebDiscoveryInfoBarBackground] = {
      nala::kColorContainerBackground};
  mixer[kColorWebDiscoveryInfoBarMessage] = {nala::kColorTextPrimary};
  mixer[kColorWebDiscoveryInfoBarLink] = {nala::kColorTextInteractive};
  mixer[kColorWebDiscoveryInfoBarNoThanks] = {nala::kColorTextTertiary};
  mixer[kColorWebDiscoveryInfoBarClose] = {nala::kColorIconSecondary};

  mixer[kColorBraveDownloadToolbarButtonActive] = {nala::kColorIconInteractive};

  mixer[kColorLocationBarHoveredShadow] = {
      SkColorSetA(SK_ColorBLACK, 0.4 * 255)};

  // Colors for HelpBubble. IDs are defined in
  // chrome/browser/ui/color/chrome_color_id.h
  mixer[kColorFeaturePromoBubbleBackground] = {nala::kColorContainerBackground};
  mixer[kColorFeaturePromoBubbleForeground] = {nala::kColorTextSecondary};
  mixer[kColorFeaturePromoBubbleCloseButtonInkDrop] = {
      kColorToolbarInkDropHover};

  mixer[kColorTabGroupBackgroundAlpha] = {
      SkColorSetA(SK_ColorBLACK, 0.25 * 255)};

  mixer[ui::kColorMenuButtonBackground] = {SK_ColorTRANSPARENT};

#if defined(TOOLKIT_VIEWS)
  if (!base::FeatureList::IsEnabled(
          darker_theme::features::kBraveDarkerTheme) ||
      key.scheme_variant != ui::ColorProviderKey::SchemeVariant::kDarker) {
    return;
  }

  auto& postprocessing_mixer = provider->AddPostprocessingMixer();
  postprocessing_mixer[kColorForTest] = {kDarkerColorForTest};
  postprocessing_mixer[ui::kColorFrameActive] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral0);
  postprocessing_mixer[ui::kColorFrameInactive] = {
      postprocessing_mixer.GetResultColor(ui::kColorFrameActive)};
  postprocessing_mixer[kColorToolbar] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral5);

  // Override saved tab group button / bookmark folder icon color in darker
  // theme. These colors are originally defined in material_chrome_color_mixer,
  // But we override them here because we want to override these only when it's
  // darker theme mode except Private/Tor profile. Also we want to make it sure
  // they are overridden regardless of order of adding mixers, so we use
  // postprocessing mixer.
  postprocessing_mixer[kColorBookmarkFolderIcon] = {
      nala::kColorPrimitiveNeutral40};
  postprocessing_mixer[kColorBookmarkButtonIcon] = {
      nala::kColorPrimitiveNeutral40};

  // Override menu separator color in darker theme from application side.
  postprocessing_mixer[ui::kColorMenuSeparator] = {
      nala::kColorPrimitiveNeutral15};
  postprocessing_mixer[kColorBraveContentsOutline] = {
      nala::kColorPrimitiveNeutral15};
#endif  // defined(TOOLKIT_VIEWS)
}

// Handling dark or light theme on normal profile.
void AddBraveThemeColorMixer(ui::ColorProvider* provider,
                             const ui::ColorProviderKey& key) {
  AddBraveColorMixerForAllThemes(provider, key);

  key.color_mode == ui::ColorProviderKey::ColorMode::kDark
      ? AddBraveDarkThemeColorMixer(provider, key)
      : AddBraveLightThemeColorMixer(provider, key);
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  AddBraveVpnColorMixer(provider, key);
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
  AddBraveSpeedreaderColorMixer(provider, key);
#endif

  auto& mixer = provider->AddMixer();
  mixer[kColorIconBase] = {nala::kColorIconDefault};
  mixer[kColorBookmarkBarInstructionsLink] = {nala::kColorTextInteractive};
  mixer[kColorSearchConversionBannerTypeBackground] = {nala::kColorBlue10};
  mixer[kColorSearchConversionCloseButton] = {nala::kColorIconDefault};
  mixer[kColorSidebarPanelHeaderTitle] = {nala::kColorTextPrimary};
  mixer[kColorSidebarPanelHeaderButton] = {nala::kColorIconDefault};
  mixer[kColorSidebarPanelHeaderButtonHovered] = {nala::kColorNeutral60};
  mixer[kColorSidebarAddBubbleBackground] = {nala::kColorContainerBackground};

  if (!HasCustomToolbarColor(key)) {
    mixer[kColorToolbarButtonActivated] = {nala::kColorIconInteractive};
  }

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  mixer[kColorWaybackMachineURLNotAvailable] = {
      nala::kColorSystemfeedbackErrorIcon};
#endif

  mixer[kColorBraveExtensionMenuIcon] = {nala::kColorIconInteractive};
  mixer[kColorBraveContentsOutline] = {
      nala::kColorDesktopbrowserToolbarButtonOutline};

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    playlist::AddThemeColorMixer(provider, key);
  }
#endif
}

void AddBravePrivateThemeColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderKey& key) {
  AddBraveDarkThemeColorMixer(provider, key);

  // Add private theme specific brave colors here.
  ui::ColorMixer& mixer = provider->AddMixer();
  mixer[kColorForTest] = {kPrivateColorForTest};

  mixer[kColorToolbarButtonActivated] = {nala::kColorPrimitivePrivateWindow80};
  mixer[kColorSidebarButtonPressed] = {kColorToolbarButtonActivated};

  mixer[kColorSidebarPanelHeaderTitle] = {nala::kColorPrimitiveNeutral90};
  mixer[kColorSidebarPanelHeaderButton] = {nala::kColorPrimitiveNeutral90};
  mixer[kColorSidebarPanelHeaderButtonHovered] = {
      nala::kColorPrimitiveNeutral80};
}

void AddBraveTorThemeColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderKey& key) {
  AddBravePrivateThemeColorMixer(provider, key);
  AddChromeDarkThemeColorMixer(provider, key);

  // Add tor theme specific brave colors here.
}

void AddPrivateThemeColorMixer(ui::ColorProvider* provider,
                               const ui::ColorProviderKey& key) {
  AddBravePrivateThemeColorMixer(provider, key);
  AddChromeDarkThemeColorMixer(provider, key);

  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorBookmarkBarForeground] = {nala::kColorPrimitivePrivateWindow100};
  mixer[kColorLocationBarFocusRing] = {nala::kColorPrimitivePrivateWindow80};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabPageBackground] = {kPrivateFrame};
  mixer[kColorTabBackgroundActiveFrameActive] = {
      nala::kColorPrimitivePrivateWindow20};
  mixer[kColorTabBackgroundActiveFrameInactive] = {
      kColorTabBackgroundActiveFrameActive};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};

  mixer[kColorTabForegroundActiveFrameActive] = {
      nala::kColorPrimitivePrivateWindow95};
  mixer[kColorTabForegroundActiveFrameInactive] = {
      nala::kColorPrimitivePrivateWindow80};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      nala::kColorPrimitivePrivateWindow80};
  mixer[kColorToolbar] = {nala::kColorPrimitivePrivateWindow20};
  mixer[kColorToolbarButtonIcon] = {nala::kColorPrimitivePrivateWindow70};
  mixer[kColorToolbarButtonIconInactive] = {
      ui::SetAlpha(kColorToolbarButtonIcon, kBraveDisabledControlAlpha)};
  mixer[ui::kColorFrameActive] = {kPrivateFrame};
  mixer[ui::kColorFrameInactive] = {kPrivateFrame};
}

void AddTorThemeColorMixer(ui::ColorProvider* provider,
                           const ui::ColorProviderKey& key) {
  AddBraveTorThemeColorMixer(provider, key);

  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorLocationBarFocusRing] = {nala::kColorPrimitiveTorWindow80};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabPageBackground] = {kPrivateTorFrame};
  mixer[kColorTabBackgroundActiveFrameActive] = {
      nala::kColorPrimitiveTorWindow20};
  mixer[kColorTabBackgroundActiveFrameInactive] = {
      kColorTabBackgroundActiveFrameActive};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};

  mixer[kColorTabForegroundActiveFrameActive] = {
      nala::kColorPrimitiveTorWindow95};
  mixer[kColorTabForegroundActiveFrameInactive] = {
      nala::kColorPrimitiveTorWindow80};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      nala::kColorPrimitiveTorWindow80};
  mixer[kColorToolbar] = {nala::kColorPrimitiveTorWindow20};
  mixer[kColorToolbarButtonIcon] = {nala::kColorPrimitiveTorWindow70};
  mixer[kColorToolbarButtonIconInactive] = {
      ui::SetAlpha(kColorToolbarButtonIcon, kBraveDisabledControlAlpha)};
  mixer[ui::kColorFrameActive] = {kPrivateTorFrame};
  mixer[ui::kColorFrameInactive] = {kPrivateTorFrame};
  mixer[kColorTabDividerFrameInactive] = {nala::kColorPrimitiveTorWindow35};
  mixer[kColorTabDividerFrameActive] = {nala::kColorPrimitiveTorWindow35};
}

void AddBraveOmniboxPrivateThemeColorMixer(ui::ColorProvider* provider,
                                           const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorToolbarBackgroundSubtleEmphasis] = {kPrivateFrame};
  mixer[kColorToolbarBackgroundSubtleEmphasisHovered] = {
      kColorToolbarBackgroundSubtleEmphasis};

  mixer[kColorOmniboxResultsBackground] = {kPrivateFrame};
  if (IsHighContrast()) {
    mixer[kColorOmniboxResultsBackground] = {
        color_utils::HSLShift(kPrivateFrame, {-1, -1, 0.45})};
  }

  AddOmniboxHoverSelect(mixer);

  mixer[kColorOmniboxIconBackground] = {SK_ColorTRANSPARENT};
  mixer[kColorOmniboxSecurityChipDangerousBackground] = {SK_ColorTRANSPARENT};
  mixer[kColorOmniboxSecurityChipText] = {kColorOmniboxSecurityChipDangerous};
}

void AddBraveOmniboxColorMixer(ui::ColorProvider* provider,
                               const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  const bool use_accent_tinted_omnibox = ShouldUseAccentTintedPalette(key);
  const bool is_dark = key.color_mode == ui::ColorProviderKey::ColorMode::kDark;

  if (!key.custom_theme) {
    mixer[kColorToolbarContentAreaSeparator] = {nala::kColorDividerSubtle};

    if (use_accent_tinted_omnibox && is_dark) {
      // Nala omnibox tokens use neutral primitives which do not visibly pick up
      // accent themes. Use the secondary ref palette instead, which is
      // generated from the user's accent color in dark mode.
      mixer[kColorLocationBarBackground] = {ui::kColorRefSecondary15};
      mixer[kColorOmniboxResultsBackground] = {ui::kColorRefSecondary20};
      mixer[kColorOmniboxResultsBackgroundHovered] = {ui::kColorRefSecondary25};
      mixer[kColorOmniboxResultsBackgroundSelected] = {
          ui::kColorRefSecondary25};
    } else if (use_accent_tinted_omnibox) {
      // Light mode uses a tinted surface token so the omnibox container
      // reflects the user's accent color.
      mixer[kColorLocationBarBackground] = {ui::kColorSysSurface4};
      mixer[kColorOmniboxResultsBackground] = {
          nala::kColorDesktopbrowserOmnibarBackgroundActive};
      mixer[kColorOmniboxResultsBackgroundHovered] = {
          nala::kColorDesktopbrowserOmnibarBackgroundHoverItem};
      mixer[kColorOmniboxResultsBackgroundSelected] = {
          nala::kColorDesktopbrowserOmnibarBackgroundSelectedItem};
    } else {
      mixer[kColorLocationBarBackground] = {
          nala::kColorDesktopbrowserOmnibarBackgroundDesktop};
      mixer[kColorOmniboxResultsBackground] = {
          nala::kColorDesktopbrowserOmnibarBackgroundActive};
      mixer[kColorOmniboxResultsBackgroundHovered] = {
          nala::kColorDesktopbrowserOmnibarBackgroundHoverItem};
      mixer[kColorOmniboxResultsBackgroundSelected] = {
          nala::kColorDesktopbrowserOmnibarBackgroundSelectedItem};
    }
  }
  mixer[kColorBraveOmniboxResultViewSeparator] = {
      kColorToolbarContentAreaSeparator};
  mixer[kColorOmniboxResultsUrl] = {nala::kColorTextInteractive};
  // We use a shadow for the Omnibox hover effect, rather than a color change.
  mixer[kColorLocationBarBackgroundHovered] = {kColorLocationBarBackground};
  mixer[kColorOmniboxIconHover] = {
      ui::SetAlpha(kColorOmniboxText, std::ceil(0.10f * 255.0f))};

  // We don't use bg color for location icon view.
  mixer[kColorOmniboxIconBackground] = {SK_ColorTRANSPARENT};
  mixer[kColorOmniboxSecurityChipDangerousBackground] = {SK_ColorTRANSPARENT};
  mixer[kColorOmniboxSecurityChipText] = {kColorOmniboxSecurityChipDangerous};

#if defined(TOOLKIT_VIEWS)
  if (!base::FeatureList::IsEnabled(
          darker_theme::features::kBraveDarkerTheme) ||
      key.scheme_variant != ui::ColorProviderKey::SchemeVariant::kDarker) {
    return;
  }

  auto& postprocessing_mixer = provider->AddPostprocessingMixer();
  // Location bar
  // Note: The darker (ultra dark) theme intentionally keeps the original
  // omnibar tokens regardless of accent color. Accent tinting only applies to
  // the regular dark-mode branch in AddBraveOmniboxColorMixer's
  // !custom_theme block.
  postprocessing_mixer[kColorLocationBarBackground] = {
      nala::kColorPrimitiveNeutral0};
  postprocessing_mixer[kColorLocationBarBackgroundHovered] = {
      nala::kColorPrimitiveNeutral0};

  // Omnibox
  postprocessing_mixer[kColorOmniboxResultsBackground] = {
      nala::kColorPrimitiveNeutral0};
  postprocessing_mixer[kColorOmniboxResultsBackgroundHovered] = {
      nala::kColorPrimitiveNeutral20};
  postprocessing_mixer[kColorOmniboxResultsBackgroundSelected] = {
      nala::kColorPrimitiveNeutral20};
  postprocessing_mixer[kColorBraveOmniboxResultViewSeparator] = {
      nala::kColorPrimitiveNeutral20};

  // Toolbar
  postprocessing_mixer[kColorToolbarButtonIcon] = {
      nala::kColorPrimitiveNeutral50};
  postprocessing_mixer[kColorToolbarButtonIconPressed] = {
      nala::kColorPrimitiveNeutral60};
  postprocessing_mixer[kColorToolbarButtonIconInactive] = {
      SkColorSetA(postprocessing_mixer.GetResultColor(kColorToolbarButtonIcon),
                  0xff * 0.6)};
  postprocessing_mixer[kColorToolbarButtonIconHovered] =
      darker_theme::ApplyDarknessFromColor(nala::kColorPrimitiveNeutral60);
  postprocessing_mixer[kColorToolbarButtonActivated] = {
      nala::kColorPrimitivePrimary60};
  postprocessing_mixer[kColorToolbarInkDrop] = {nala::kColorPrimitiveNeutral10};
  postprocessing_mixer[kColorToolbarInkDropHover] = {
      nala::kColorPrimitiveNeutral10};
  postprocessing_mixer[kColorToolbarInkDropRipple] = {
      nala::kColorPrimitiveNeutral20};

  // Sidebar button
  postprocessing_mixer[kColorSidebarButtonBase] = {
      postprocessing_mixer.GetResultColor(kColorToolbarButtonIcon)};
  if (!HasCustomToolbarColor(key)) {
    mixer[kColorSidebarButtonPressed] = {
        postprocessing_mixer.GetResultColor(kColorToolbarButtonActivated)};
  }

  if (!key.custom_theme) {
    mixer[kColorToolbarContentAreaSeparator] = {
        postprocessing_mixer.GetResultColor(kColorToolbar)};
  }
#endif  // defined(TOOLKIT_VIEWS)
}

void AddBravifiedTabStripColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  // TabStripComboButton's bg color should be different per horizontal tab mode.
  // When horizontal tab, that color should be same with title bar's bg color.
  // When vertical tab, that color should be toolbar's bg color.
  // To make TabStripComboButton have same bg color with title bar or toolbar,
  // simply set its bg color transparent.
  // If this causes other buttons' bg color regressions, we need to change
  // combo button's bg color directly.
  mixer[kColorNewTabButtonCRBackgroundFrameActive] = {SK_ColorTRANSPARENT};
  mixer[kColorNewTabButtonCRBackgroundFrameInactive] = {SK_ColorTRANSPARENT};

  if (key.custom_theme) {
    return;
  }

  mixer[kColorNewTabButtonFocusRing] = {ui::kColorFocusableBorderFocused};
  mixer[kColorTabBackgroundActiveFrameActive] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameInactive] = {
      kColorTabBackgroundActiveFrameActive};
  mixer[kColorTabDividerFrameActive] =
      ui::AlphaBlend({nala::kColorDividerStrong},
                     {kColorTabBackgroundInactiveFrameActive}, 0.75 * 0xff);
  mixer[kColorTabDividerFrameInactive] = {kColorTabDividerFrameActive};

  // Other tab colors are handled in brave_tab_color_mixer.cc
}
