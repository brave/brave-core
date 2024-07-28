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
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/playlist/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/color/chrome_color_provider_utils.h"
#include "chrome/browser/ui/color/material_chrome_color_mixer.h"
#include "chrome/browser/ui/color/material_side_panel_color_mixer.h"
#include "ui/base/ui_base_features.h"
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
constexpr SkColor kPrivateLocationBarBgBase = kPrivateFrame;
constexpr SkColor kLightLocationBarBgBase = SK_ColorWHITE;
constexpr SkColor kDarkLocationBarBgBase = kDarkFrame;

// Copied from //chrome/browser/ui/omnibox//omnibox_theme.h
// As below values are not changed for several years, it would be safe to copy.
// Can't include //chrome/browser/ui/omnibox in here due to circular deps.
constexpr float kOmniboxOpacityHovered = 0.10f;
constexpr float kOmniboxOpacitySelected = 0.16f;

SkColor PickColorContrastingToOmniboxResultsBackground(
    const ui::ColorProviderKey& key,
    const ui::ColorMixer& mixer,
    SkColor color1,
    SkColor color2) {
  auto bg_color = mixer.GetResultColor(kColorOmniboxResultsBackground);
  return color_utils::PickContrastingColor(color1, color2, bg_color);
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

  const bool is_dark = key.color_mode == ui::ColorProviderKey::ColorMode::kDark;
  const bool has_custom_theme = !!key.custom_theme;
  if (has_custom_theme) {
    // TODO(simonhong): Use proper vpn bg/border colors with custom theme.
    mixer[kColorBraveVpnButtonBorder] = {PickSimilarColorToToolbar(
        key, mixer, SkColorSetARGB(0x14, 0x13, 0x16, 0x20),
        SkColorSetARGB(0x4D, 0x04, 0x04, 0x06))};
    mixer[kColorBraveVpnButtonBackgroundHover] = {PickSimilarColorToToolbar(
        key, mixer, SkColorSetARGB(0x14, 0x13, 0x16, 0x20),
        SkColorSetARGB(0x4D, 0x04, 0x04, 0x06))};
  } else {
    mixer[kColorBraveVpnButtonBorder] = {
        leo::GetColor(leo::Color::kColorDividerSubtle,
                      is_dark ? leo::Theme::kDark : leo::Theme::kLight)};
    // TODO(simonhong): Use leo color. button/Background-active is not available
    // yet.
    mixer[kColorBraveVpnButtonBackgroundHover] = {
        is_dark ? SkColorSetRGB(0x0D, 0x0F, 0x14)
                : SkColorSetRGB(0xDB, 0xDE, 0xE2)};
  }
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
      leo::GetColor(leo::Color::kColorIconDefault, leo::Theme::kLight),
      leo::GetColor(leo::Color::kColorIconDefault, leo::Theme::kDark))};

  mixer[kColorSpeedreaderToolbarButtonHover] = {PickSimilarColorToToolbar(
      key, mixer, SkColorSetARGB(0x0D, 0x13, 0x16, 0x20),
      SkColorSetARGB(0x59, 0x0A, 0x0B, 0x10))};
  mixer[kColorSpeedreaderToolbarButtonActive] = {PickSimilarColorToToolbar(
      key, mixer, SkColorSetARGB(0x14, 0x13, 0x16, 0x20),
      SkColorSetARGB(0x80, 0x0A, 0x0B, 0x10))};
  mixer[kColorSpeedreaderToolbarButtonActiveText] = {PickSimilarColorToToolbar(
      key, mixer,
      leo::GetColor(leo::Color::kColorIconInteractive, leo::Theme::kLight),
      leo::GetColor(leo::Color::kColorIconInteractive, leo::Theme::kDark))};
  mixer[kColorSpeedreaderToolbarButtonBorder] = {PickSimilarColorToToolbar(
      key, mixer,
      leo::GetColor(leo::Color::kColorDividerSubtle, leo::Theme::kLight),
      leo::GetColor(leo::Color::kColorDividerSubtle, leo::Theme::kDark))};
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
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorTabForegroundActiveFrameActive] = {
      leo::GetColor(leo::Color::kColorTextPrimary, leo::Theme::kLight)};
  mixer[kColorTabForegroundActiveFrameInactive] = {
      leo::GetColor(leo::Color::kColorTextPrimary, leo::Theme::kLight)};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      leo::GetColor(leo::Color::kColorTextSecondary, leo::Theme::kLight)};
  mixer[kColorTabStrokeFrameActive] = {SkColorSetA(SK_ColorBLACK, 0.07 * 255)};
  mixer[kColorTabStrokeFrameInactive] = {kColorTabStrokeFrameActive};
  mixer[kColorToolbar] = {leo::kColorPrimitiveNeutral98};
  mixer[kColorToolbarButtonIcon] = {leo::kColorPrimitiveNeutral50};
  mixer[kColorToolbarButtonIconInactive] = {
      ui::SetAlpha(kColorToolbarButtonIcon, kBraveDisabledControlAlpha)};
  mixer[kColorToolbarContentAreaSeparator] = {ui::kColorFrameActive};
  mixer[kColorToolbarTopSeparatorFrameActive] = {kColorToolbar};
  mixer[kColorToolbarTopSeparatorFrameInactive] = {kColorToolbar};
  mixer[ui::kColorFrameActive] = {kLightFrame};
  // TODO(simonhong): Should we adjust frame color for inactive window?
  mixer[ui::kColorFrameInactive] = {kLightFrame};
  mixer[ui::kColorToggleButtonThumbOff] = {SK_ColorWHITE};
  mixer[ui::kColorToggleButtonThumbOn] = {SkColorSetRGB(0x4C, 0x54, 0xD2)};
  mixer[ui::kColorToggleButtonTrackOff] = {SkColorSetRGB(0xDA, 0xDC, 0xE8)};
  mixer[ui::kColorToggleButtonTrackOn] = {SkColorSetRGB(0xE1, 0xE2, 0xF6)};

  // Used for download button progress ring color.
  mixer[kColorDownloadToolbarButtonActive] = {SkColorSetRGB(0x42, 0x3E, 0xEE)};
  mixer[kColorDownloadToolbarButtonRingBackground] = {
      SkColorSetARGB(0x0F, 0x1D, 0x1F, 0x25)};

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
  mixer[kColorOmniboxChipBackground] = {kLightFrame};
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
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorTabForegroundActiveFrameActive] = {
      leo::GetColor(leo::Color::kColorTextPrimary, leo::Theme::kDark)};
  mixer[kColorTabForegroundActiveFrameInactive] = {
      leo::GetColor(leo::Color::kColorTextPrimary, leo::Theme::kDark)};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      leo::GetColor(leo::Color::kColorTextSecondary, leo::Theme::kDark)};
  mixer[kColorTabStrokeFrameActive] = {kColorToolbar};
  mixer[kColorTabStrokeFrameInactive] = {kColorToolbar};
  mixer[kColorToolbar] = {leo::kColorPrimitiveNeutral10};
  mixer[kColorToolbarButtonIcon] = {leo::kColorPrimitiveNeutral70};
  mixer[kColorToolbarButtonIconInactive] = {
      ui::SetAlpha(kColorToolbarButtonIcon, kBraveDisabledControlAlpha)};
  mixer[kColorToolbarContentAreaSeparator] = {kColorToolbar};
  mixer[kColorToolbarTopSeparatorFrameActive] = {kColorToolbar};
  mixer[kColorToolbarTopSeparatorFrameInactive] = {kColorToolbar};
  mixer[ui::kColorFrameActive] = {kDarkFrame};
  mixer[ui::kColorFrameInactive] = {kDarkFrame};
  mixer[ui::kColorToggleButtonThumbOff] = {SK_ColorWHITE};
  mixer[ui::kColorToggleButtonThumbOn] = {SkColorSetRGB(0x44, 0x36, 0xE1)};
  mixer[ui::kColorToggleButtonTrackOff] = {SkColorSetRGB(0x5E, 0x61, 0x75)};
  mixer[ui::kColorToggleButtonTrackOn] = {SkColorSetRGB(0x76, 0x79, 0xB1)};
  mixer[kColorDownloadToolbarButtonActive] = {SkColorSetRGB(0x87, 0x84, 0xF4)};
  mixer[kColorDownloadToolbarButtonRingBackground] = {
      SkColorSetARGB(0x33, 0x16, 0x17, 0x1D)};

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

  if (key.custom_theme) {
    return;
  }

  mixer[kColorLocationBarBackground] = {kColorToolbarBackgroundSubtleEmphasis};
  mixer[kColorLocationBarBackgroundHovered] = {kColorLocationBarBackground};

  // We don't show border when omnibox doesn't have focus but still
  // contains in-progress user input.
  mixer[kColorLocationBarBorderOnMismatch] = {SK_ColorTRANSPARENT};
}

void AddBraveColorMixerForAllThemes(ui::ColorProvider* provider,
                                    const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  // Custom theme will use this color. Other themes could have another
  // color by their color mixers.
  mixer[kColorToolbarButtonActivated] = {SkColorSetRGB(0x7C, 0x91, 0xFF)};
  mixer[kColorSidebarButtonPressed] = {kColorToolbarButtonActivated};
}

void AddBraveOmniboxLightThemeColorMixer(ui::ColorProvider* provider,
                                         const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  // Apply bravified color when there is no custom theme.
  mixer[kColorToolbarBackgroundSubtleEmphasis] = {GetLocationBarBackground(
      /*dark*/ false, /*private*/ false)};
  // Use same color for normal & hover location bar background.
  // Instead, shadow is set when hovered.
  mixer[kColorToolbarBackgroundSubtleEmphasisHovered] = {
      kColorToolbarBackgroundSubtleEmphasis};
  mixer[kColorOmniboxText] = {kLightOmniboxText};

  mixer[kColorOmniboxResultsBackground] = {GetOmniboxResultBackground(
      kColorOmniboxResultsBackground, /*dark*/ false, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundHovered] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundHovered,
                                 /*dark*/ false, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundSelected] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundSelected,
                                 /*dark*/ false, /*incognito*/ false)};
  mixer[kColorOmniboxResultsFocusIndicator] = {
      ui::kColorFocusableBorderFocused};
  mixer[kColorOmniboxResultsUrl] = {
      leo::GetColor(leo::Color::kColorTextInteractive, leo::Theme::kLight)};
  mixer[kColorOmniboxResultsUrlSelected] = {kColorOmniboxResultsUrl};
}

void AddBraveOmniboxDarkThemeColorMixer(ui::ColorProvider* provider,
                                        const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  // Apply bravified color when there is no custom theme.
  mixer[kColorToolbarBackgroundSubtleEmphasis] = {GetLocationBarBackground(
      /*dark*/ true, /*private*/ false)};
  mixer[kColorToolbarBackgroundSubtleEmphasisHovered] = {
      kColorToolbarBackgroundSubtleEmphasis};
  mixer[kColorOmniboxText] = {kDarkOmniboxText};

  mixer[kColorOmniboxResultsBackground] = {GetOmniboxResultBackground(
      kColorOmniboxResultsBackground, /*dark*/ true, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundHovered] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundHovered,
                                 /*dark*/ true, /*incognito*/ false)};
  mixer[kColorOmniboxResultsBackgroundSelected] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundSelected,
                                 /*dark*/ true, /*incognito*/ false)};
  mixer[kColorOmniboxResultsFocusIndicator] = {
      ui::kColorFocusableBorderFocused};
  mixer[kColorOmniboxResultsUrl] = {
      leo::GetColor(leo::Color::kColorTextInteractive, leo::Theme::kDark)};
  mixer[kColorOmniboxResultsUrlSelected] = {kColorOmniboxResultsUrl};
}

}  // namespace

SkColor GetLocationBarBackground(bool dark, bool priv) {
  if (priv) {
    return kPrivateLocationBarBgBase;
  }

  if (dark) {
    return kDarkLocationBarBgBase;
  }

  return kLightLocationBarBgBase;
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
  AddChromeColorMixerForAllThemes(provider, key);

  AddMaterialChromeColorMixer(provider, key);
  AddMaterialSidePanelColorMixer(provider, key);

  // TODO(simonhong): Use leo color when it's ready.
  // TODO(simonhong): Move these overriding to
  // AddChromeColorMixerForAllThemes().
  ui::ColorMixer& mixer = provider->AddMixer();
  mixer[kColorAppMenuHighlightSeverityLow] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x00, 0x46, 0x07),
                                    SkColorSetRGB(0x58, 0xE1, 0x55))};
  mixer[kColorAppMenuHighlightSeverityMedium] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x4A, 0x39, 0x00),
                                    SkColorSetRGB(0xF1, 0xC0, 0x0F))};
  mixer[kColorAppMenuHighlightSeverityHigh] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x7D, 0x00, 0x1A),
                                    SkColorSetRGB(0xFF, 0xB3, 0xB2))};

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
  mixer[kColorBookmarkBarInstructionsLink] = {PickColorContrastingToToolbar(
      key, mixer, leo::light::kColorTextInteractive,
      leo::dark::kColorTextInteractive)};
  mixer[kColorMenuItemSubText] = {SkColorSetRGB(0x86, 0x8E, 0x96)};
  // It's "Themeable/Blue/10" but leo/color.h doesn't have it.
  mixer[kColorSearchConversionBannerTypeBackground] = {
      SkColorSetRGB(0xEA, 0xF1, 0xFF)};
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
  mixer[kColorSidebarArrowBackgroundHovered] = {kColorToolbarInkDropHover};
  mixer[kColorSidebarSeparator] = {SkColorSetRGB(0xE6, 0xE8, 0xF5)};
  mixer[kColorSidebarPanelHeaderSeparator] = {
      leo::GetColor(leo::Color::kColorDividerSubtle, leo::Theme::kLight)};
  mixer[kColorSidebarPanelHeaderBackground] = {
      leo::GetColor(leo::Color::kColorContainerBackground, leo::Theme::kLight)};
  mixer[kColorSidebarPanelHeaderTitle] = {
      leo::GetColor(leo::Color::kColorTextPrimary, leo::Theme::kLight)};
  mixer[kColorSidebarPanelHeaderButton] = {
      leo::GetColor(leo::Color::kColorIconDefault, leo::Theme::kLight)};
  mixer[kColorSidebarPanelHeaderButtonHovered] = {
      leo::GetColor(leo::Color::kColorNeutral60, leo::Theme::kLight)};

  mixer[kColorSidebarButtonBase] = {kColorToolbarButtonIcon};
  if (!HasCustomToolbarColor(key)) {
    mixer[kColorToolbarButtonActivated] = {
        leo::GetColor(leo::Color::kColorIconInteractive, leo::Theme::kLight)};
    mixer[kColorSidebarButtonPressed] = {kColorToolbarButtonActivated};
  }

  mixer[kColorSidebarAddButtonDisabled] = {PickColorContrastingToToolbar(
      key, mixer, SkColorSetARGB(0x66, 0x49, 0x50, 0x57),
      SkColorSetARGB(0x66, 0xC2, 0xC4, 0xCF))};

  mixer[kColorSidebarArrowDisabled] = {PickColorContrastingToToolbar(
      key, mixer, SkColorSetARGB(0x8A, 0x49, 0x50, 0x57),
      SkColorSetARGB(0x8A, 0xAE, 0xB1, 0xC2))};
  mixer[kColorSidebarArrowNormal] = {kColorSidebarButtonBase};
  mixer[kColorSidebarItemDragIndicator] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x21, 0x25, 0x29),
                                    SkColorSetRGB(0xC2, 0xC4, 0xCF))};
  mixer[kColorWebDiscoveryInfoBarBackground] = {
      SkColorSetRGB(0xFF, 0xFF, 0xFF)};
  mixer[kColorWebDiscoveryInfoBarMessage] = {SkColorSetRGB(0x1D, 0x1F, 0x25)};
  mixer[kColorWebDiscoveryInfoBarLink] = {SkColorSetRGB(0x4C, 0x54, 0xD2)};
  mixer[kColorWebDiscoveryInfoBarNoThanks] = {SkColorSetRGB(0x6B, 0x70, 0x84)};
  mixer[kColorWebDiscoveryInfoBarClose] = {SkColorSetRGB(0x6B, 0x70, 0x84)};

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  mixer[kColorWaybackMachineURLLoaded] = {leo::GetColor(
      leo::Color::kColorSystemfeedbackSuccessIcon, leo::Theme::kLight)};
  mixer[kColorWaybackMachineURLNotAvailable] = {leo::GetColor(
      leo::Color::kColorSystemfeedbackErrorIcon, leo::Theme::kLight)};
#endif

  // Color for download button when all completed and button needs user
  // interaction.
  mixer[kColorBraveDownloadToolbarButtonActive] = {
      SkColorSetRGB(0x5F, 0x5C, 0xF1)};

  mixer[kColorLocationBarHoveredShadow] = {
      SkColorSetA(SK_ColorBLACK, 0.07 * 255)};

  // Colors for HelpBubble. IDs are defined in
  // chrome/browser/ui/color/chrome_color_id.h
  mixer[kColorFeaturePromoBubbleBackground] = {SK_ColorWHITE};
  mixer[kColorFeaturePromoBubbleForeground] = {SkColorSetRGB(0x42, 0x45, 0x52)};
  mixer[kColorFeaturePromoBubbleCloseButtonInkDrop] = {
      kColorToolbarInkDropHover};

  mixer[kColorTabGroupBackgroundAlpha] = {
      SkColorSetA(SK_ColorBLACK, 0.15 * 255)};

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    playlist::AddThemeColorMixer(provider, leo::Theme::kLight, key);
  }
#endif

  mixer[kColorBraveExtensionMenuIcon] = {
      leo::GetColor(leo::Color::kColorIconInteractive, leo::Theme::kLight)};

  mixer[kColorBraveAppMenuAccentColor] = {SkColorSetRGB(0xDF, 0xE1, 0xFF)};
}

void AddBraveDarkThemeColorMixer(ui::ColorProvider* provider,
                                 const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorForTest] = {kDarkColorForTest};

  mixer[kColorIconBase] = {SkColorSetRGB(0xC2, 0xC4, 0xCF)};
  mixer[kColorBookmarkBarInstructionsText] = {
      PickColorContrastingToToolbar(key, mixer, SkColorSetRGB(0x49, 0x50, 0x57),
                                    SkColorSetRGB(0xFF, 0xFF, 0xFF))};
  mixer[kColorBookmarkBarInstructionsLink] = {PickColorContrastingToToolbar(
      key, mixer, leo::light::kColorTextInteractive,
      leo::dark::kColorTextInteractive)};
  mixer[kColorMenuItemSubText] = {SkColorSetRGB(0x84, 0x88, 0x9C)};
  // It's "Themeable/Blue/10" but leo/color.h doesn't have it.
  mixer[kColorSearchConversionBannerTypeBackground] = {
      SkColorSetRGB(0x00, 0x1C, 0x37)};
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
  mixer[kColorSidebarArrowBackgroundHovered] = {kColorToolbarInkDropHover};
  mixer[kColorSidebarSeparator] = {SkColorSetRGB(0x5E, 0x61, 0x75)};
  mixer[kColorSidebarPanelHeaderSeparator] = {
      leo::GetColor(leo::Color::kColorDividerSubtle, leo::Theme::kDark)};

  // To align with upstream's panel backround color, use |kGogleGreay900|.
  // When we apply our style to panel webui, use below color for header.
  // leo::GetColor(leo::Color::kColorContainerBackground, leo::Theme::kDark).
  // Or delete when panel webui renders header view also.
  mixer[kColorSidebarPanelHeaderBackground] = {gfx::kGoogleGrey900};
  mixer[kColorSidebarPanelHeaderTitle] = {
      leo::GetColor(leo::Color::kColorTextPrimary, leo::Theme::kDark)};
  mixer[kColorSidebarPanelHeaderButton] = {
      leo::GetColor(leo::Color::kColorIconDefault, leo::Theme::kDark)};
  mixer[kColorSidebarPanelHeaderButtonHovered] = {
      leo::GetColor(leo::Color::kColorNeutral60, leo::Theme::kDark)};

  mixer[kColorSidebarButtonBase] = {kColorToolbarButtonIcon};
  if (!HasCustomToolbarColor(key)) {
    mixer[kColorToolbarButtonActivated] = {
        leo::GetColor(leo::Color::kColorIconInteractive, leo::Theme::kDark)};
    mixer[kColorSidebarButtonPressed] = {kColorToolbarButtonActivated};
  }
  mixer[kColorSidebarAddButtonDisabled] = {PickColorContrastingToToolbar(
      key, mixer, SkColorSetARGB(0x66, 0x49, 0x50, 0x57),
      SkColorSetARGB(0x66, 0xC2, 0xC4, 0xCF))};
  mixer[kColorSidebarArrowDisabled] = {PickColorContrastingToToolbar(
      key, mixer, SkColorSetARGB(0x8A, 0x49, 0x50, 0x57),
      SkColorSetARGB(0x8A, 0xAE, 0xB1, 0xC2))};
  mixer[kColorSidebarArrowNormal] = {kColorSidebarButtonBase};
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

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  mixer[kColorWaybackMachineURLLoaded] = {leo::GetColor(
      leo::Color::kColorSystemfeedbackSuccessIcon, leo::Theme::kDark)};
  mixer[kColorWaybackMachineURLNotAvailable] = {leo::GetColor(
      leo::Color::kColorSystemfeedbackErrorIcon, leo::Theme::kDark)};
#endif

  mixer[kColorBraveDownloadToolbarButtonActive] = {
      SkColorSetRGB(0x87, 0x84, 0xF4)};

  mixer[kColorLocationBarHoveredShadow] = {
      SkColorSetA(SK_ColorBLACK, 0.4 * 255)};

  // Colors for HelpBubble. IDs are defined in
  // chrome/browser/ui/color/chrome_color_id.h
  mixer[kColorFeaturePromoBubbleBackground] = {SkColorSetRGB(0x12, 0x13, 0x16)};
  mixer[kColorFeaturePromoBubbleForeground] = {SkColorSetRGB(0xC6, 0xC8, 0xD0)};
  mixer[kColorFeaturePromoBubbleCloseButtonInkDrop] = {
      kColorToolbarInkDropHover};

  mixer[kColorTabGroupBackgroundAlpha] = {
      SkColorSetA(SK_ColorBLACK, 0.25 * 255)};

#if BUILDFLAG(ENABLE_PLAYLIST_WEBUI)
  if (base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    playlist::AddThemeColorMixer(provider, leo::Theme::kDark, key);
  }
#endif

  mixer[kColorBraveExtensionMenuIcon] = {
      leo::GetColor(leo::Color::kColorIconInteractive, leo::Theme::kDark)};

  mixer[kColorBraveAppMenuAccentColor] = {SkColorSetRGB(0x37, 0x2C, 0xBF)};
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
}

void AddBravePrivateThemeColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderKey& key) {
  AddBraveDarkThemeColorMixer(provider, key);

  // Add private theme specific brave colors here.
  ui::ColorMixer& mixer = provider->AddMixer();
  mixer[kColorForTest] = {kPrivateColorForTest};

  mixer[kColorToolbarButtonActivated] = {SkColorSetRGB(0x7C, 0x91, 0xFF)};
  mixer[kColorSidebarButtonPressed] = {kColorToolbarButtonActivated};

  // |key.color_mode| always dark as we use dark native theme for
  // private/tor/guest profile. See BraveBrowserFrame::GetNativeTheme().
  // Exceptionally, below side panel header colors should be brave theme
  // specific because side panel header colors should be aligned with
  // side panel contents.
  const bool is_dark = dark_mode::GetActiveBraveDarkModeType() ==
                       dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  mixer[kColorSidebarPanelHeaderSeparator] = {
      leo::GetColor(leo::Color::kColorDividerSubtle,
                    is_dark ? leo::Theme::kDark : leo::Theme::kLight)};
  mixer[kColorSidebarPanelHeaderBackground] = {
      is_dark ? gfx::kGoogleGrey900
              : leo::GetColor(leo::Color::kColorContainerBackground,
                              leo::Theme::kLight)};
  mixer[kColorSidebarPanelHeaderTitle] = {
      leo::GetColor(leo::Color::kColorTextPrimary,
                    is_dark ? leo::Theme::kDark : leo::Theme::kLight)};
  mixer[kColorSidebarPanelHeaderButton] = {
      leo::GetColor(leo::Color::kColorIconDefault,
                    is_dark ? leo::Theme::kDark : leo::Theme::kLight)};
  mixer[kColorSidebarPanelHeaderButtonHovered] = {
      leo::GetColor(leo::Color::kColorNeutral60,
                    is_dark ? leo::Theme::kDark : leo::Theme::kLight)};
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
  mixer[kColorTabBackgroundActiveFrameActive] = {
      leo::kColorPrimitivePrivateWindow20};
  mixer[kColorTabBackgroundActiveFrameInactive] = {
      kColorTabBackgroundActiveFrameActive};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};

  // TODO(simonhong): Get color from leo when it's available.
  mixer[kColorTabForegroundActiveFrameActive] = {
      SkColorSetRGB(0xF5, 0xF3, 0xFF)};
  mixer[kColorTabForegroundActiveFrameInactive] = {
      SkColorSetRGB(0xCC, 0xBE, 0xFE)};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      SkColorSetRGB(0xCC, 0xBE, 0xFE)};
  mixer[kColorToolbar] = {leo::kColorPrimitivePrivateWindow10};
  mixer[kColorToolbarButtonIcon] = {leo::kColorPrimitivePrivateWindow70};
  mixer[kColorToolbarButtonIconInactive] = {
      ui::SetAlpha(kColorToolbarButtonIcon, kBraveDisabledControlAlpha)};
  mixer[kColorToolbarContentAreaSeparator] = {kColorToolbar};
  mixer[ui::kColorFrameActive] = {kPrivateFrame};
  mixer[ui::kColorFrameInactive] = {kPrivateFrame};
}

void AddTorThemeColorMixer(ui::ColorProvider* provider,
                           const ui::ColorProviderKey& key) {
  AddBraveTorThemeColorMixer(provider, key);

  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorLocationBarFocusRing] = {SkColorSetRGB(0xCF, 0xAB, 0xE2)};
  mixer[kColorNewTabButtonBackgroundFrameActive] = {ui::kColorFrameActive};
  mixer[kColorNewTabButtonBackgroundFrameInactive] = {ui::kColorFrameInactive};
  mixer[kColorNewTabPageBackground] = {kPrivateTorFrame};
  mixer[kColorTabBackgroundActiveFrameActive] = {
      leo::kColorPrimitiveTorWindow20};
  mixer[kColorTabBackgroundActiveFrameInactive] = {
      kColorTabBackgroundActiveFrameActive};
  mixer[kColorTabBackgroundInactiveFrameActive] = {ui::kColorFrameActive};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {ui::kColorFrameInactive};

  // TODO(simonhong): Get color from leo when it's available.
  mixer[kColorTabForegroundActiveFrameActive] = {
      SkColorSetRGB(0xFA, 0xF3, 0xFF)};
  mixer[kColorTabForegroundActiveFrameInactive] = {
      SkColorSetRGB(0xE3, 0xB3, 0xFF)};
  mixer[kColorTabForegroundInactiveFrameActive] = {
      SkColorSetRGB(0xE3, 0xB3, 0xFF)};
  mixer[kColorToolbar] = {leo::kColorPrimitiveTorWindow10};
  mixer[kColorToolbarButtonIcon] = {leo::kColorPrimitiveTorWindow70};
  mixer[kColorToolbarButtonIconInactive] = {
      ui::SetAlpha(kColorToolbarButtonIcon, kBraveDisabledControlAlpha)};
  mixer[kColorToolbarContentAreaSeparator] = {kColorToolbar};
  mixer[ui::kColorFrameActive] = {kPrivateTorFrame};
  mixer[ui::kColorFrameInactive] = {kPrivateTorFrame};
}

void AddBraveOmniboxPrivateThemeColorMixer(ui::ColorProvider* provider,
                                           const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorToolbarBackgroundSubtleEmphasis] = {GetLocationBarBackground(
      /*dark*/ false, /*private*/ true)};
  mixer[kColorToolbarBackgroundSubtleEmphasisHovered] = {
      kColorToolbarBackgroundSubtleEmphasis};
  mixer[kColorOmniboxText] = {kDarkOmniboxText};

  mixer[kColorOmniboxResultsBackground] = {GetOmniboxResultBackground(
      kColorOmniboxResultsBackground, /*dark*/ false, /*incognito*/ true)};
  mixer[kColorOmniboxResultsBackgroundHovered] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundHovered,
                                 /*dark*/ false, /*incognito*/ true)};
  mixer[kColorOmniboxResultsBackgroundSelected] = {
      GetOmniboxResultBackground(kColorOmniboxResultsBackgroundSelected,
                                 /*dark*/ false, /*incognito*/ true)};
  mixer[kColorPageInfoBackground] = {SK_ColorTRANSPARENT};
}

void AddBraveOmniboxColorMixer(ui::ColorProvider* provider,
                               const ui::ColorProviderKey& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  auto pick_color = [&](leo::Color color) {
    if (!key.custom_theme) {
      return leo::GetColor(color, leo::Theme::kDark);
    }

    return PickColorContrastingToOmniboxResultsBackground(
        key, mixer, leo::GetColor(color, leo::Theme::kLight),
        leo::GetColor(color, leo::Theme::kDark));
  };

  mixer[kColorBraveOmniboxResultViewSeparator] = {
      pick_color(leo::Color::kColorDividerSubtle)};
  mixer[kColorBravePlayerActionViewBorder] = {
      pick_color(leo::Color::kColorDividerSubtle)};

  // Re-apply non-material color.
  mixer[kColorOmniboxResultsButtonBorder] = ui::BlendTowardMaxContrast(
      kColorToolbarBackgroundSubtleEmphasis, gfx::kGoogleGreyAlpha400);
  mixer[kColorOmniboxResultsButtonIcon] = {kColorOmniboxResultsIcon};
  mixer[kColorOmniboxResultsButtonIconSelected] = {
      kColorOmniboxResultsIconSelected};
  mixer[kColorPageInfoIconHover] = {
      ui::SetAlpha(kColorOmniboxText, std::ceil(0.10f * 255.0f))};

  // We don't use bg color for location icon view.
  mixer[kColorPageInfoBackground] = {SK_ColorTRANSPARENT};
  if (key.custom_theme) {
    return;
  }

  key.color_mode == ui::ColorProviderKey::ColorMode::kDark
      ? AddBraveOmniboxDarkThemeColorMixer(provider, key)
      : AddBraveOmniboxLightThemeColorMixer(provider, key);
}

void AddBravifiedTabStripColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderKey& key) {
  if (key.custom_theme) {
    return;
  }

  ui::ColorMixer& mixer = provider->AddMixer();
  const bool is_dark = key.color_mode == ui::ColorProviderKey::ColorMode::kDark;

  mixer[kColorNewTabButtonFocusRing] = {ui::kColorFocusableBorderFocused};
  mixer[kColorTabBackgroundActiveFrameActive] = {
      is_dark ? leo::kColorPrimitiveNeutral20 : SK_ColorWHITE};
  mixer[kColorTabBackgroundActiveFrameInactive] = {
      kColorTabBackgroundActiveFrameActive};
}
