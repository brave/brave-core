/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/color/brave_color_mixer.h"

#include "base/notreached.h"
#include "base/numerics/safe_conversions.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/browser/ui/color/leo/colors.h"
#include "brave/browser/ui/tabs/brave_vertical_tab_color_mixer.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
#include "brave/browser/ui/color/playlist/playlist_color_mixer.h"
#include "brave/components/playlist/common/features.h"
#endif

namespace {

// Location bar colors
const SkColor kPrivateLocationBarBgBase = SkColorSetRGB(0x0B, 0x07, 0x24);
const SkColor kDarkLocationBarBgBase = SkColorSetRGB(0x18, 0x1A, 0x21);
const SkColor kDarkLocationBarHoverBg = SkColorSetRGB(0x23, 0x25, 0x2F);

// Copied from //chrome/browser/ui/omnibox//omnibox_theme.h
// As below values are not changed for several years, it would be safe to copy.
// Can't include //chrome/browser/ui/omnibox in here due to circular deps.
constexpr float kOmniboxOpacityHovered = 0.10f;
constexpr float kOmniboxOpacitySelected = 0.16f;

SkColor GetToolbarInkDropColor(const ui::ColorMixer& mixer) {
  // Copied from
  // chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h
  // to use same hover background with toolbar button.
  constexpr float kToolbarInkDropHighlightVisibleOpacity = 0.08f;
  return SkColorSetA(mixer.GetResultColor(kColorToolbarInkDrop),
                     0xFF * kToolbarInkDropHighlightVisibleOpacity);
}

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

#if BUILDFLAG(ENABLE_BRAVE_VPN)
void AddBraveVpnColorMixer(ui::ColorProvider* provider,
                           const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorBraveVpnButtonText] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x1C, 0x1E, 0x26),
                                    SkColorSetRGB(0xED, 0xEE, 0xF1))};
  mixer[kColorBraveVpnButtonTextError] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0xDC, 0x1D, 0x3C),
                                    SkColorSetRGB(0xEB, 0x63, 0x7A))};

  mixer[kColorBraveVpnButtonBorder] = {PickSimilarColorToToolbar(
      key, mixer, SkColorSetARGB(0x14, 0x13, 0x16, 0x20),
      SkColorSetARGB(0x4D, 0x04, 0x04, 0x06))};
  mixer[kColorBraveVpnButtonErrorBorder] = {kColorBraveVpnButtonTextError};

  mixer[kColorBraveVpnButtonIconConnected] = {SkColorSetRGB(0x3F, 0xA4, 0x50)};
  mixer[kColorBraveVpnButtonIconDisconnected] = {PickColorContrastingToToolbar(
      key, mixer, SkColorSetARGB(0x99, 0x0B, 0x16, 0x41),
      SkColorSetARGB(0xCC, 0xB1, 0xB7, 0xCD))};
  mixer[kColorBraveVpnButtonIconInner] = {PickSimilarColorToToolbar(
      key, mixer, SK_ColorWHITE, SkColorSetARGB(0x33, 0x04, 0x04, 0x06))};
  mixer[kColorBraveVpnButtonIconError] = {kColorBraveVpnButtonErrorBorder};
  mixer[kColorBraveVpnButtonIconErrorInner] = {PickSimilarColorToToolbar(
      key, mixer, SK_ColorWHITE, SkColorSetRGB(0x0F, 0x17, 0x2A))};

  mixer[kColorBraveVpnButtonBackgroundNormal] = {kColorToolbar};
  mixer[kColorBraveVpnButtonBackgroundHover] = {PickSimilarColorToToolbar(
      key, mixer, SkColorSetARGB(0x14, 0x13, 0x16, 0x20),
      SkColorSetARGB(0x4D, 0x04, 0x04, 0x06))};
  mixer[kColorBraveVpnButtonErrorBackgroundNormal] = {PickSimilarColorToToolbar(
      key, mixer, SkColorSetARGB(0x31, 0xDC, 0x1D, 0x3C),
      SkColorSetARGB(0x33, 0xEB, 0x63, 0x7A))};
  mixer[kColorBraveVpnButtonErrorBackgroundHover] = {PickSimilarColorToToolbar(
      key, mixer, SkColorSetARGB(0x40, 0xDC, 0x1D, 0x3C),
      SkColorSetARGB(0x40, 0xEB, 0x63, 0x7A))};
}
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
void AddBraveSpeedreaderColorMixer(ui::ColorProvider* provider,
                                   const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorSpeedreaderIcon] = {
      PickSimilarColorToToolbar(key, mixer, SkColorSetRGB(0x4C, 0x54, 0xD2),
                                SkColorSetRGB(0x73, 0x7A, 0xDE))};
  mixer[kColorSpeedreaderToggleThumb] = {
      PickSimilarColorToToolbar(key, mixer, SkColorSetRGB(0x4C, 0x54, 0xD2),
                                SkColorSetRGB(0x44, 0x36, 0xE1))};
  mixer[kColorSpeedreaderToggleTrack] = {
      PickSimilarColorToToolbar(key, mixer, SkColorSetRGB(0xE1, 0xE2, 0xF6),
                                SkColorSetRGB(0x76, 0x79, 0xB1))};

  mixer[kColorSpeedreaderToolbarBackground] = {kColorToolbar};
  mixer[kColorSpeedreaderToolbarBorder] = {kColorToolbarContentAreaSeparator};
  mixer[kColorSpeedreaderToolbarForeground] = {PickColorContrastingToToolbar(
      key, mixer,
      leo::GetColor(leo::Color::kColorLegacyText2, leo::Theme::kLight),
      leo::GetColor(leo::Color::kColorLegacyText2, leo::Theme::kDark))};

  mixer[kColorSpeedreaderToolbarButtonHover] = {PickSimilarColorToToolbar(
      key, mixer, SkColorSetARGB(0x0D, 0x13, 0x16, 0x20),
      SkColorSetARGB(0x59, 0x0A, 0x0B, 0x10))};
  mixer[kColorSpeedreaderToolbarButtonActive] = {PickSimilarColorToToolbar(
      key, mixer, SkColorSetARGB(0x14, 0x13, 0x16, 0x20),
      SkColorSetARGB(0x80, 0x0A, 0x0B, 0x10))};
  mixer[kColorSpeedreaderToolbarButtonBorder] = {PickSimilarColorToToolbar(
      key, mixer, leo::GetColor(leo::Color::kColorGray20, leo::Theme::kLight),
      leo::GetColor(leo::Color::kColorGray10, leo::Theme::kDark))};
}
#endif

void AddChromeLightThemeColorMixer(ui::ColorProvider* provider,
                                   const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorTabThrobber] = {SkColorSetRGB(0xd7, 0x55, 0x26)};
  mixer[kColorBookmarkBarForeground] = {kColorTabForegroundActiveFrameActive};
  mixer[ui::kColorBadgeBackground] = {SkColorSetRGB(95, 92, 241)};
  mixer[ui::kColorBadgeForeground] = {SkColorSetRGB(245, 244, 254)};
  mixer[kColorDownloadShelfButtonText] = {gfx::kBraveGrey800};
  mixer[kColorForTest] = {kLightColorForTest};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabPageBackground] = {kBraveNewTabBackgroundLight};
  mixer[kColorTabBackgroundActiveFrameActive] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameInactive] = {kColorToolbar};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorTabForegroundActiveFrameActive] = {kLightToolbarIcon};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      kColorTabForegroundActiveFrameActive};
  mixer[kColorToolbar] = {kLightToolbar};
  mixer[kColorToolbarButtonIcon] = {kColorTabForegroundActiveFrameActive};
  mixer[kColorToolbarButtonIconInactive] = {
      color_utils::AlphaBlend(kLightToolbarIcon, kLightToolbar, 0.3f)};
  mixer[kColorToolbarContentAreaSeparator] = {ui::kColorFrameActive};
  mixer[kColorToolbarTopSeparatorFrameActive] = {kColorToolbar};
  mixer[kColorToolbarTopSeparatorFrameInactive] = {kColorToolbar};
  mixer[ui::kColorFrameActive] = {kLightFrame};
  mixer[ui::kColorFrameInactive] = {
      color_utils::HSLShift(kLightFrame, {-1, -1, 0.6})};
  mixer[ui::kColorToggleButtonThumbOff] = {SK_ColorWHITE};
  mixer[ui::kColorToggleButtonThumbOn] = {SkColorSetRGB(0x4C, 0x54, 0xD2)};
  mixer[ui::kColorToggleButtonTrackOff] = {SkColorSetRGB(0xDA, 0xDC, 0xE8)};
  mixer[ui::kColorToggleButtonTrackOn] = {SkColorSetRGB(0xE1, 0xE2, 0xF6)};

  // Used for download button progress ring color.
  mixer[kColorDownloadToolbarButtonActive] = {SkColorSetRGB(0x42, 0x3E, 0xEE)};
  mixer[kColorDownloadToolbarButtonRingBackground] = {
      SkColorSetARGB(0x0F, 0x1D, 0x1F, 0x25)};
}

void AddChromeDarkThemeColorMixer(ui::ColorProvider* provider,
                                  const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorTabThrobber] = {SkColorSetRGB(0xd7, 0x55, 0x26)};
  mixer[kColorBookmarkBarForeground] = {kColorTabForegroundActiveFrameActive};
  mixer[ui::kColorBadgeBackground] = {SkColorSetRGB(135, 132, 244)};
  mixer[ui::kColorBadgeForeground] = {SkColorSetRGB(14, 14, 52)};
  mixer[kColorDownloadShelfButtonText] = {SK_ColorWHITE};
  mixer[kColorForTest] = {kDarkColorForTest};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabPageBackground] = {kBraveNewTabBackgroundDark};
  mixer[kColorTabBackgroundActiveFrameActive] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameInactive] = {kColorToolbar};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorTabForegroundActiveFrameActive] = {
      SkColorSetRGB(0xF3, 0xF3, 0xF3)};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      kColorTabForegroundActiveFrameActive};
  mixer[kColorTabStrokeFrameActive] = {kColorToolbar};
  mixer[kColorTabStrokeFrameInactive] = {kColorToolbar};
  mixer[kColorToolbar] = {kDarkToolbar};
  mixer[kColorToolbarButtonIcon] = {kDarkToolbarIcon};
  mixer[kColorToolbarButtonIconInactive] = {
      color_utils::AlphaBlend(kDarkToolbarIcon, kDarkToolbar, 0.3f)};
  mixer[kColorToolbarContentAreaSeparator] = {kColorToolbar};
  mixer[kColorToolbarTopSeparatorFrameActive] = {kColorToolbar};
  mixer[kColorToolbarTopSeparatorFrameInactive] = {kColorToolbar};
  mixer[ui::kColorFrameActive] = {kDarkFrame};
  mixer[ui::kColorFrameInactive] = {
      color_utils::HSLShift(kDarkFrame, {-1, -1, 0.6})};
  mixer[ui::kColorToggleButtonThumbOff] = {SK_ColorWHITE};
  mixer[ui::kColorToggleButtonThumbOn] = {SkColorSetRGB(0x44, 0x36, 0xE1)};
  mixer[ui::kColorToggleButtonTrackOff] = {SkColorSetRGB(0x5E, 0x61, 0x75)};
  mixer[ui::kColorToggleButtonTrackOn] = {SkColorSetRGB(0x76, 0x79, 0xB1)};
  mixer[kColorDownloadToolbarButtonActive] = {SkColorSetRGB(0x87, 0x84, 0xF4)};
  mixer[kColorDownloadToolbarButtonRingBackground] = {
      SkColorSetARGB(0x33, 0x16, 0x17, 0x1D)};
}

}  // namespace

SkColor GetLocationBarBackground(bool dark, bool priv, bool hover) {
  if (priv) {
    return hover ? color_utils::HSLShift(kPrivateLocationBarBgBase,
                                         {-1, -1, 0.54})
                 : kPrivateLocationBarBgBase;
  }

  if (dark) {
    return hover ? kDarkLocationBarHoverBg : kDarkLocationBarBgBase;
  }

  return hover ? color_utils::AlphaBlend(SK_ColorWHITE,
                                         SkColorSetRGB(0xf3, 0xf3, 0xf3), 0.7f)
               : SK_ColorWHITE;
}

// Omnibox result bg colors
SkColor GetOmniboxResultBackground(int id, bool dark, bool priv) {
#if !defined(USE_AURA)
  ui::NativeTheme* native_theme = nullptr;
#else
  // For high contrast, selected rows use inverted colors to stand out more.
  ui::NativeTheme* native_theme = ui::NativeTheme::GetInstanceForNativeUi();
#endif  // !defined(USE_AURA)
  bool high_contrast =
      native_theme && native_theme->UserHasContrastPreference();
  float omnibox_opacity = 0.0f;
  if (id == kColorOmniboxResultsBackgroundHovered) {
    omnibox_opacity = kOmniboxOpacityHovered;
  } else if (id == kColorOmniboxResultsBackgroundSelected) {
    omnibox_opacity = kOmniboxOpacitySelected;
  }

  SkColor color;
  if (priv) {
    color = high_contrast ? color_utils::HSLShift(kPrivateLocationBarBgBase,
                                                  {-1, -1, 0.45})
                          : kPrivateLocationBarBgBase;
  } else if (dark) {
    color = high_contrast ? gfx::kGoogleGrey900 : kDarkLocationBarBgBase;
  } else {
    color = SK_ColorWHITE;
  }
  return color_utils::BlendTowardMaxContrast(
      color, base::ClampRound(omnibox_opacity * 0xff));
}

void AddBravifiedChromeThemeColorMixer(ui::ColorProvider* provider,
                                       const ui::ColorProviderKey& key) {
  if (key.custom_theme) {
    return;
  }

  key.color_mode == ui::ColorProviderKey::ColorMode::kDark
      ? AddChromeDarkThemeColorMixer(provider, key)
      : AddChromeLightThemeColorMixer(provider, key);
}

void AddBraveLightThemeColorMixer(ui::ColorProvider* provider,
                                  const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorForTest] = {kLightColorForTest};

  mixer[kColorIconBase] = {SkColorSetRGB(0x49, 0x50, 0x57)};
  mixer[kColorBookmarkBarInstructionsText] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x49, 0x50, 0x57),
                                    SkColorSetRGB(0xFF, 0xFF, 0xFF))};
  mixer[kColorMenuItemSubText] = {SkColorSetRGB(0x86, 0x8E, 0x96)};
  mixer[kColorSearchConversionCloseButton] = {
      leo::GetColor(leo::Color::kColorIconDefault, leo::Theme::kLight)};
  mixer[kColorSearchConversionBannerTypeDescText] = {
      SkColorSetRGB(0x2E, 0x30, 0x39)};
  mixer[kColorSearchConversionBannerTypeBackgroundBorder] = {
      SkColorSetRGB(0xE2, 0xE3, 0xF8)};
  mixer[kColorSearchConversionBannerTypeBackgroundBorderHovered] = {
      SkColorSetRGB(0x83, 0x89, 0xE0)};
  mixer[kColorSearchConversionBannerTypeBackgroundGradientFrom] = {
      SkColorSetARGB(104, 0xFF, 0xFF, 0xFF)};
  mixer[kColorSearchConversionBannerTypeBackgroundGradientTo] = {
      SkColorSetRGB(0xEF, 0xEF, 0xFB)};
  mixer[kColorSearchConversionButtonTypeInputAppend] = {
      SkColorSetRGB(0x58, 0x5C, 0x6D)};
  mixer[kColorSearchConversionButtonTypeBackgroundNormal] = {
      SkColorSetRGB(0xED, 0xEE, 0xFA)};
  mixer[kColorSearchConversionButtonTypeBackgroundHovered] = {
      SkColorSetRGB(0xE2, 0xE3, 0xF8)};

  mixer[kColorSearchConversionButtonTypeDescNormal] = {
      SkColorSetRGB(0x44, 0x4d, 0xd0)};
  mixer[kColorSearchConversionButtonTypeDescHovered] = {
      SkColorSetRGB(0x1F, 0x25, 0x7A)};
  mixer[kColorDialogDontAskAgainButton] = {SkColorSetRGB(0x86, 0x8E, 0x96)};
  mixer[kColorDialogDontAskAgainButtonHovered] = {
      SkColorSetRGB(0x49, 0x50, 0x57)};
  mixer[kColorSidebarAddBubbleBackground] = {SK_ColorWHITE};
  mixer[kColorSidebarAddBubbleHeaderText] = {SkColorSetRGB(0x17, 0x17, 0x1F)};
  mixer[kColorSidebarAddBubbleItemTextBackgroundHovered] = {
      SkColorSetRGB(0x4C, 0x54, 0xD2)};
  mixer[kColorSidebarAddBubbleItemTextHovered] = {
      SkColorSetRGB(0xF0, 0xF2, 0xFF)};
  mixer[kColorSidebarAddBubbleItemTextNormal] = {
      SkColorSetRGB(0x21, 0x25, 0x29)};
  mixer[kColorSidebarArrowBackgroundHovered] = {GetToolbarInkDropColor(mixer)};
  mixer[kColorSidebarItemBackgroundHovered] = {GetToolbarInkDropColor(mixer)};
  mixer[kColorSidebarSeparator] = {SkColorSetRGB(0xE6, 0xE8, 0xF5)};

  mixer[kColorSidebarButtonBase] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x49, 0x50, 0x57),
                                    SkColorSetRGB(0xC2, 0xC4, 0xCF))};
  mixer[kColorSidebarAddButtonDisabled] = {PickColorContrastingToToolbar(
      key, mixer, SkColorSetARGB(0x66, 0x49, 0x50, 0x57),
      SkColorSetARGB(0x66, 0xC2, 0xC4, 0xCF))};
  mixer[kColorSidebarArrowDisabled] = {PickColorContrastingToToolbar(
      key, mixer, SkColorSetARGB(0x8A, 0x49, 0x50, 0x57),
      SkColorSetARGB(0x8A, 0xAE, 0xB1, 0xC2))};
  mixer[kColorSidebarArrowNormal] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x21, 0x25, 0x29),
                                    SkColorSetRGB(0xC2, 0xC4, 0xCF))};
  mixer[kColorSidebarItemDragIndicator] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x21, 0x25, 0x29),
                                    SkColorSetRGB(0xC2, 0xC4, 0xCF))};
  mixer[kColorWebDiscoveryInfoBarBackground] = {
      SkColorSetRGB(0xFF, 0xFF, 0xFF)};
  mixer[kColorWebDiscoveryInfoBarMessage] = {SkColorSetRGB(0x1D, 0x1F, 0x25)};
  mixer[kColorWebDiscoveryInfoBarLink] = {SkColorSetRGB(0x4C, 0x54, 0xD2)};
  mixer[kColorWebDiscoveryInfoBarNoThanks] = {SkColorSetRGB(0x6B, 0x70, 0x84)};
  mixer[kColorWebDiscoveryInfoBarClose] = {SkColorSetRGB(0x6B, 0x70, 0x84)};

  // Color for download button when all completed and button needs user
  // interaction.
  mixer[kColorBraveDownloadToolbarButtonActive] = {
      SkColorSetRGB(0x5F, 0x5C, 0xF1)};

  // Colors for HelpBubble. IDs are defined in
  // chrome/browser/ui/color/chrome_color_id.h
  mixer[kColorFeaturePromoBubbleBackground] = {SK_ColorWHITE};
  mixer[kColorFeaturePromoBubbleForeground] = {SkColorSetRGB(0x42, 0x45, 0x52)};
  mixer[kColorFeaturePromoBubbleCloseButtonInkDrop] = {
      GetToolbarInkDropColor(mixer)};

  if (base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    tabs::AddBraveVerticalTabLightThemeColorMixer(provider, key);
  }

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    playlist::AddThemeColorMixer(provider, leo::Theme::kLight, key);
  }
#endif
}

void AddBraveDarkThemeColorMixer(ui::ColorProvider* provider,
                                 const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorForTest] = {kDarkColorForTest};

  mixer[kColorIconBase] = {SkColorSetRGB(0xC2, 0xC4, 0xCF)};
  mixer[kColorBookmarkBarInstructionsText] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x49, 0x50, 0x57),
                                    SkColorSetRGB(0xFF, 0xFF, 0xFF))};
  mixer[kColorMenuItemSubText] = {SkColorSetRGB(0x84, 0x88, 0x9C)};
  mixer[kColorSearchConversionCloseButton] = {
      leo::GetColor(leo::Color::kColorIconDefault, leo::Theme::kDark)};
  mixer[kColorSearchConversionBannerTypeDescText] = {
      SkColorSetRGB(0xE2, 0xE3, 0xE7)};
  mixer[kColorSearchConversionBannerTypeBackgroundBorder] = {
      SkColorSetRGB(0x1F, 0x25, 0x7A)};
  mixer[kColorSearchConversionBannerTypeBackgroundBorderHovered] = {
      SkColorSetRGB(0x5F, 0x67, 0xD7)};
  mixer[kColorSearchConversionBannerTypeBackgroundGradientFrom] = {
      SkColorSetARGB(104, 0x17, 0x19, 0x1E)};
  mixer[kColorSearchConversionBannerTypeBackgroundGradientTo] = {
      SkColorSetARGB(104, 0x1F, 0x25, 0x7A)};
  mixer[kColorSearchConversionButtonTypeInputAppend] = {
      SkColorSetRGB(0xAC, 0xAF, 0xBB)};
  mixer[kColorSearchConversionButtonTypeBackgroundNormal] = {
      SkColorSetRGB(0x1A, 0x1C, 0x3B)};
  mixer[kColorSearchConversionButtonTypeBackgroundHovered] = {
      SkColorSetRGB(0x1F, 0x25, 0x7A)};
  mixer[kColorSearchConversionButtonTypeDescNormal] = {
      SkColorSetRGB(0xA6, 0xAB, 0xE9)};
  mixer[kColorSearchConversionButtonTypeDescHovered] = {
      SkColorSetRGB(0xE2, 0xE3, 0xF8)};
  mixer[kColorDialogDontAskAgainButton] = {SkColorSetRGB(0x84, 0x88, 0x9C)};
  mixer[kColorDialogDontAskAgainButtonHovered] = {
      SkColorSetRGB(0xC2, 0xC4, 0xCF)};
  mixer[kColorSidebarAddBubbleBackground] = {gfx::kBraveGrey800};
  mixer[kColorSidebarAddBubbleHeaderText] = {SkColorSetRGB(0xF0, 0xF0, 0xFF)};
  mixer[kColorSidebarAddBubbleItemTextBackgroundHovered] = {
      SkColorSetRGB(0x4C, 0x54, 0xD2)};
  mixer[kColorSidebarAddBubbleItemTextHovered] = {
      SkColorSetRGB(0xF0, 0xF0, 0xFF)};
  mixer[kColorSidebarAddBubbleItemTextNormal] = {
      SkColorSetRGB(0xF0, 0xF0, 0xFF)};
  mixer[kColorSidebarArrowBackgroundHovered] = {GetToolbarInkDropColor(mixer)};
  mixer[kColorSidebarItemBackgroundHovered] = {GetToolbarInkDropColor(mixer)};
  mixer[kColorSidebarSeparator] = {SkColorSetRGB(0x5E, 0x61, 0x75)};
  mixer[kColorSidebarButtonBase] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x49, 0x50, 0x57),
                                    SkColorSetRGB(0xC2, 0xC4, 0xCF))};
  mixer[kColorSidebarAddButtonDisabled] = {PickColorContrastingToToolbar(
      key, mixer, SkColorSetARGB(0x66, 0x49, 0x50, 0x57),
      SkColorSetARGB(0x66, 0xC2, 0xC4, 0xCF))};
  mixer[kColorSidebarArrowDisabled] = {PickColorContrastingToToolbar(
      key, mixer, SkColorSetARGB(0x8A, 0x49, 0x50, 0x57),
      SkColorSetARGB(0x8A, 0xAE, 0xB1, 0xC2))};
  mixer[kColorSidebarArrowNormal] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x21, 0x25, 0x29),
                                    SkColorSetRGB(0xC2, 0xC4, 0xCF))};
  mixer[kColorSidebarItemDragIndicator] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x21, 0x25, 0x29),
                                    SkColorSetRGB(0xC2, 0xC4, 0xCF))};
  mixer[kColorWebDiscoveryInfoBarBackground] = {
      SkColorSetRGB(0x1A, 0x1C, 0x22)};
  mixer[kColorWebDiscoveryInfoBarMessage] = {SkColorSetRGB(0xFF, 0xFF, 0xFF)};
  mixer[kColorWebDiscoveryInfoBarLink] = {SkColorSetRGB(0xA6, 0xAB, 0xEC)};
  mixer[kColorWebDiscoveryInfoBarNoThanks] = {
      SkColorSetARGB(0xBF, 0xEC, 0xEF, 0xF2)};
  mixer[kColorWebDiscoveryInfoBarClose] = {
      SkColorSetARGB(0xBF, 0x8C, 0x90, 0xA1)};

  mixer[kColorBraveDownloadToolbarButtonActive] = {
      SkColorSetRGB(0x87, 0x84, 0xF4)};

  // Colors for HelpBubble. IDs are defined in
  // chrome/browser/ui/color/chrome_color_id.h
  mixer[kColorFeaturePromoBubbleBackground] = {SkColorSetRGB(0x12, 0x13, 0x16)};
  mixer[kColorFeaturePromoBubbleForeground] = {SkColorSetRGB(0xC6, 0xC8, 0xD0)};
  mixer[kColorFeaturePromoBubbleCloseButtonInkDrop] = {
      GetToolbarInkDropColor(mixer)};

  if (base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    tabs::AddBraveVerticalTabDarkThemeColorMixer(provider, key);
  }

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    playlist::AddThemeColorMixer(provider, leo::Theme::kDark, key);
  }
#endif
}

// Handling dark or light theme on normal profile.
void AddBraveThemeColorMixer(ui::ColorProvider* provider,
                             const ui::ColorProviderKey& key) {
  key.color_mode == ui::ColorProviderKey::ColorMode::kDark
      ? AddBraveDarkThemeColorMixer(provider, key)
      : AddBraveLightThemeColorMixer(provider, key);
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  AddBraveVpnColorMixer(provider, key);
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
  AddBraveSpeedreaderColorMixer(provider, key);
#endif
}

void AddBravePrivateThemeColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderKey& key) {
  AddBraveDarkThemeColorMixer(provider, key);

  // Add private theme specific brave colors here.
  ui::ColorMixer& mixer = provider->AddMixer();
  mixer[kColorForTest] = {kPrivateColorForTest};
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

  mixer[kColorBookmarkBarForeground] = {SkColorSetRGB(0xFF, 0xFF, 0xFF)};
  mixer[kColorLocationBarFocusRing] = {SkColorSetRGB(0xC6, 0xB3, 0xFF)};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabPageBackground] = {kPrivateFrame};
  mixer[kColorTabBackgroundActiveFrameActive] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameInactive] = {kColorToolbar};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorTabForegroundActiveFrameActive] = {
      SkColorSetRGB(0xF3, 0xF3, 0xF3)};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      SkColorSetRGB(0xFF, 0xFF, 0xFF)};
  mixer[kColorToolbar] = {kPrivateToolbar};
  mixer[kColorToolbarButtonIcon] = {kDarkToolbarIcon};
  mixer[kColorToolbarButtonIconInactive] = {
      color_utils::AlphaBlend(kDarkToolbarIcon, kPrivateToolbar, 0.3f)};
  mixer[kColorToolbarContentAreaSeparator] = {kColorToolbar};
  mixer[ui::kColorFrameActive] = {kPrivateFrame};
  mixer[ui::kColorFrameInactive] = {
      color_utils::HSLShift(kPrivateFrame, {-1, -1, 0.55})};
}

void AddTorThemeColorMixer(ui::ColorProvider* provider,
                           const ui::ColorProviderKey& key) {
  AddBraveTorThemeColorMixer(provider, key);

  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorLocationBarFocusRing] = {SkColorSetRGB(0xCF, 0xAB, 0xE2)};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabPageBackground] = {kPrivateTorFrame};
  mixer[kColorTabBackgroundActiveFrameActive] = {kColorToolbar};
  mixer[kColorTabBackgroundActiveFrameInactive] = {kColorToolbar};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorToolbar] = {kPrivateTorToolbar};
  mixer[kColorToolbarButtonIconInactive] = {
      color_utils::AlphaBlend(kDarkToolbarIcon, kPrivateTorToolbar, 0.3f)};
  mixer[kColorToolbarContentAreaSeparator] = {kColorToolbar};
  mixer[ui::kColorFrameActive] = {kPrivateTorFrame};
  mixer[ui::kColorFrameInactive] = {
      color_utils::HSLShift(kPrivateTorFrame, {-1, -1, 0.55})};
}

void AddBraveOmniboxLightThemeColorMixer(ui::ColorProvider* provider,
                                         const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorToolbarBackgroundSubtleEmphasis] = {GetLocationBarBackground(
      /*dark*/ false, /*private*/ false, /*hover*/ false)};
  mixer[kColorToolbarBackgroundSubtleEmphasisHovered] = {
      GetLocationBarBackground(
          /*dark*/ false, /*private*/ false, /*hover*/ true)};
  mixer[kColorOmniboxText] = {kLightOmniboxText};

  mixer[kColorOmniboxResultsBackground] = {GetOmniboxResultBackground(
      kColorOmniboxResultsBackground, /*dark*/ false, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundHovered] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundHovered,
                                 /*dark*/ false, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundSelected] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundSelected,
                                 /*dark*/ false, /*incognito*/ false)};
}

void AddBraveOmniboxDarkThemeColorMixer(ui::ColorProvider* provider,
                                        const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorToolbarBackgroundSubtleEmphasis] = {GetLocationBarBackground(
      /*dark*/ true, /*private*/ false, /*hover*/ false)};
  mixer[kColorToolbarBackgroundSubtleEmphasisHovered] = {
      GetLocationBarBackground(
          /*dark*/ true, /*private*/ false, /*hover*/ true)};
  mixer[kColorOmniboxText] = {kDarkOmniboxText};

  mixer[kColorOmniboxResultsBackground] = {GetOmniboxResultBackground(
      kColorOmniboxResultsBackground, /*dark*/ true, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundHovered] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundHovered,
                                 /*dark*/ true, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundSelected] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundSelected,
                                 /*dark*/ true, /*incognito*/ false)};
}

void AddBraveOmniboxPrivateThemeColorMixer(ui::ColorProvider* provider,
                                           const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorToolbarBackgroundSubtleEmphasis] = {GetLocationBarBackground(
      /*dark*/ false, /*private*/ true, /*hover*/ false)};
  mixer[kColorToolbarBackgroundSubtleEmphasisHovered] = {
      GetLocationBarBackground(
          /*dark*/ false, /*private*/ true, /*hover*/ true)};
  mixer[kColorOmniboxText] = {kDarkOmniboxText};

  mixer[kColorOmniboxResultsBackground] = {GetOmniboxResultBackground(
      kColorOmniboxResultsBackground, /*dark*/ false, /*incognito*/ true)};
  mixer[kColorOmniboxResultsBackgroundHovered] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundHovered,
                                 /*dark*/ false, /*incognito*/ true)};
  mixer[kColorOmniboxResultsBackgroundSelected] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundSelected,
                                 /*dark*/ false, /*incognito*/ true)};
}
