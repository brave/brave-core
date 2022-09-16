/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/color/brave_color_mixer.h"

#include "base/notreached.h"
#include "base/numerics/safe_conversions.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

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

enum class ProfileType {
  kNormalProfile,
  kPrivateProfile,
  kTorProfile,
};

struct ColorPropertiesMapEntry {
  int property_id;
  BraveColorIds color_id;
};

SkColor GetColorContrastingToToolbar(const ui::ColorProviderManager::Key& key,
                                     const ui::ColorMixer& mixer,
                                     const ColorPropertiesMapEntry& entry,
                                     ProfileType profile_type) {
  // Custom theme's toolbar color will be the final color.
  auto toolbar_color = mixer.GetResultColor(kColorToolbar);
  SkColor custom_toolbar_color;
  if (key.custom_theme &&
      key.custom_theme->GetColor(ThemeProperties::COLOR_TOOLBAR,
                                 &custom_toolbar_color)) {
    toolbar_color = custom_toolbar_color;
  }
  const auto base_button_color_light = MaybeGetDefaultColorForBraveUi(
      entry.property_id, profile_type == ProfileType::kPrivateProfile,
      profile_type == ProfileType::kTorProfile,
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  const auto base_button_color_dark = MaybeGetDefaultColorForBraveUi(
      entry.property_id, profile_type == ProfileType::kPrivateProfile,
      profile_type == ProfileType::kTorProfile,
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  DCHECK(base_button_color_light && base_button_color_dark);
  return color_utils::PickContrastingColor(base_button_color_light.value(),
                                           base_button_color_dark.value(),
                                           toolbar_color);
}

SkColor GetBraveThemeColor(const ui::ColorProviderManager::Key& key,
                           const ColorPropertiesMapEntry& entry,
                           ProfileType profile_type) {
  auto color = MaybeGetDefaultColorForBraveUi(
      entry.property_id, profile_type == ProfileType::kPrivateProfile,
      profile_type == ProfileType::kTorProfile,
      key.color_mode == ui::ColorProviderManager::ColorMode::kLight
          ? dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT
          : dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  DCHECK(color);
  return color.value();
}

void AddBraveFeaturesColors(const ui::ColorProviderManager::Key& key,
                            ui::ColorMixer* color_mixer,
                            ProfileType profile_type) {
  static constexpr ColorPropertiesMapEntry kPropertiesMap[] = {
    {BraveThemeProperties::COLOR_ICON_BASE, kColorIconBase},
    {BraveThemeProperties::COLOR_BOOKMARK_BAR_INSTRUCTIONS_TEXT,
     kColorBookmarkBarInstructionsText},
    {BraveThemeProperties::COLOR_MENU_ITEM_SUB_TEXT_COLOR,
     kColorMenuItemSubText},
    {BraveThemeProperties::
         COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_BORDER,
     kColorSearchConversionBannerTypeBackgroundBorder},
    {BraveThemeProperties::
         COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_BORDER_HOVERED,
     kColorSearchConversionBannerTypeBackgroundBorderHovered},
    {BraveThemeProperties::
         COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_GRADIENT_FROM,
     kColorSearchConversionBannerTypeBackgroundGradientFrom},
    {BraveThemeProperties::
         COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_GRADIENT_TO,
     kColorSearchConversionBannerTypeBackgroundGradientTo},
    {BraveThemeProperties::
         COLOR_SEARCH_CONVERSION_BUTTON_TYPE_BACKGROUND_NORMAL,
     kColorSearchConversionButtonTypeBackgroundNormal},
    {BraveThemeProperties::
         COLOR_SEARCH_CONVERSION_BUTTON_TYPE_BACKGROUND_HOVERED,
     kColorSearchConversionButtonTypeBackgroundHovered},
    {BraveThemeProperties::COLOR_SEARCH_CONVERSION_BANNER_TYPE_DESC_TEXT,
     kColorSearchConversionBannerTypeDescText},
    {BraveThemeProperties::COLOR_SEARCH_CONVERSION_BUTTON_TYPE_DESC_NORMAL,
     kColorSearchConversionButtonTypeDescNormal},
    {BraveThemeProperties::COLOR_SEARCH_CONVERSION_BUTTON_TYPE_DESC_HOVERED,
     kColorSearchConversionButtonTypeDescHovered},
    {BraveThemeProperties::COLOR_SEARCH_CONVERSION_BUTTON_TYPE_INPUT_APPEND,
     kColorSearchConversionButtonTypeInputAppend},
#if BUILDFLAG(ENABLE_SIDEBAR)
    {BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_BACKGROUND,
     kColorSidebarAddBubbleBackground},
    {BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_HEADER_TEXT,
     kColorSidebarAddBubbleHeaderText},
    {BraveThemeProperties::
         COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_BACKGROUND_HOVERED,
     kColorSidebarAddBubbleItemTextBackgroundHovered},
    {BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_HOVERED,
     kColorSidebarAddBubbleItemTextHovered},
    {BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_NORMAL,
     kColorSidebarAddBubbleItemTextNormal},
    {BraveThemeProperties::COLOR_SIDEBAR_ADD_BUTTON_DISABLED,
     kColorSidebarAddButtonDisabled},
    {BraveThemeProperties::COLOR_SIDEBAR_ARROW_BACKGROUND_HOVERED,
     kColorSidebarArrowBackgroundHovered},
    {BraveThemeProperties::COLOR_SIDEBAR_ARROW_DISABLED,
     kColorSidebarArrowDisabled},
    {BraveThemeProperties::COLOR_SIDEBAR_ARROW_NORMAL,
     kColorSidebarArrowNormal},
    {BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE, kColorSidebarButtonBase},
    {BraveThemeProperties::COLOR_SIDEBAR_ITEM_BACKGROUND_HOVERED,
     kColorSidebarItemBackgroundHovered},
    {BraveThemeProperties::COLOR_SIDEBAR_ITEM_DRAG_INDICATOR_COLOR,
     kColorSidebarItemDragIndicatorColor},
    {BraveThemeProperties::COLOR_SIDEBAR_SEPARATOR, kColorSidebarSeparator},
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
    {BraveThemeProperties::COLOR_SPEEDREADER_ICON, kColorSpeedreaderIcon},
    {BraveThemeProperties::COLOR_SPEEDREADER_TOGGLE_THUMB,
     kColorSpeedreaderToggleThumb},
    {BraveThemeProperties::COLOR_SPEEDREADER_TOGGLE_TRACK,
     kColorSpeedreaderToggleTrack},
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    {BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_BORDER,
     kColorBraveVpnButtonBorder},
    {BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_CONNECTED,
     kColorBraveVpnButtonTextConnected},
    {BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_DISCONNECTED,
     kColorBraveVpnButtonTextDisconnected},
#endif
  };

  ui::ColorMixer& mixer = *color_mixer;
  for (auto& entry : kPropertiesMap) {
    switch (entry.property_id) {
      case BraveThemeProperties::COLOR_ICON_BASE:
      case BraveThemeProperties::COLOR_MENU_ITEM_SUB_TEXT_COLOR:
      case BraveThemeProperties::
          COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_BORDER:
      case BraveThemeProperties::
          COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_BORDER_HOVERED:
      case BraveThemeProperties::
          COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_GRADIENT_FROM:
      case BraveThemeProperties::
          COLOR_SEARCH_CONVERSION_BANNER_TYPE_BACKGROUND_GRADIENT_TO:
      case BraveThemeProperties::COLOR_SEARCH_CONVERSION_BANNER_TYPE_DESC_TEXT:
      case BraveThemeProperties::
          COLOR_SEARCH_CONVERSION_BUTTON_TYPE_BACKGROUND_NORMAL:
      case BraveThemeProperties::
          COLOR_SEARCH_CONVERSION_BUTTON_TYPE_BACKGROUND_HOVERED:
      case BraveThemeProperties::
          COLOR_SEARCH_CONVERSION_BUTTON_TYPE_DESC_NORMAL:
      case BraveThemeProperties::
          COLOR_SEARCH_CONVERSION_BUTTON_TYPE_DESC_HOVERED:
      case BraveThemeProperties::
          COLOR_SEARCH_CONVERSION_BUTTON_TYPE_INPUT_APPEND: {
        mixer[entry.color_id] = {GetBraveThemeColor(key, entry, profile_type)};
        break;
      }
      case BraveThemeProperties::COLOR_BOOKMARK_BAR_INSTRUCTIONS_TEXT: {
        mixer[entry.color_id] = {
            GetColorContrastingToToolbar(key, mixer, entry, profile_type)};
        break;
      }
#if BUILDFLAG(ENABLE_SIDEBAR)
      case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_BACKGROUND:
      case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_HEADER_TEXT:
      case BraveThemeProperties::
          COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_BACKGROUND_HOVERED:
      case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_HOVERED:
      case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUBBLE_ITEM_TEXT_NORMAL:
      case BraveThemeProperties::COLOR_SIDEBAR_SEPARATOR: {
        mixer[entry.color_id] = {GetBraveThemeColor(key, entry, profile_type)};
        break;
      }
      case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUTTON_DISABLED:
      case BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE:
      case BraveThemeProperties::COLOR_SIDEBAR_ARROW_DISABLED:
      case BraveThemeProperties::COLOR_SIDEBAR_ARROW_NORMAL:
      case BraveThemeProperties::COLOR_SIDEBAR_ITEM_DRAG_INDICATOR_COLOR: {
        mixer[entry.color_id] = {
            GetColorContrastingToToolbar(key, mixer, entry, profile_type)};
        break;
      }
      case BraveThemeProperties::COLOR_SIDEBAR_ARROW_BACKGROUND_HOVERED:
      case BraveThemeProperties::COLOR_SIDEBAR_ITEM_BACKGROUND_HOVERED: {
        // Copied from
        // chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h
        // to use same hover background with toolbar button.
        constexpr float kToolbarInkDropHighlightVisibleOpacity = 0.08f;
        mixer[entry.color_id] = {
            SkColorSetA(mixer.GetResultColor(kColorToolbarInkDrop),
                        0xFF * kToolbarInkDropHighlightVisibleOpacity)};
        break;
      }
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
      case BraveThemeProperties::COLOR_SPEEDREADER_ICON:
      case BraveThemeProperties::COLOR_SPEEDREADER_TOGGLE_THUMB:
      case BraveThemeProperties::COLOR_SPEEDREADER_TOGGLE_TRACK: {
        mixer[entry.color_id] = {GetBraveThemeColor(key, entry, profile_type)};
        break;
      }
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
      case BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_BORDER: {
        mixer[entry.color_id] = {GetBraveThemeColor(key, entry, profile_type)};
        break;
      }
      case BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_CONNECTED:
      case BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_DISCONNECTED: {
        mixer[entry.color_id] = {
            GetColorContrastingToToolbar(key, mixer, entry, profile_type)};
        break;
      }
#endif
      default:
        NOTREACHED();
    }
  }
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

void AddBraveLightThemeColorMixer(ui::ColorProvider* provider,
                                  const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorBookmarkBarForeground] = {kColorTabForegroundActiveFrameActive};
  mixer[kColorDownloadShelfButtonText] = {gfx::kBraveGrey800};
  mixer[kColorForTest] = {SkColorSetRGB(0xFF, 0xFF, 0xFF)};
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

  AddBraveFeaturesColors(key, &mixer, ProfileType::kNormalProfile);
}

void AddBraveDarkThemeColorMixer(ui::ColorProvider* provider,
                                 const ui::ColorProviderManager::Key& key) {
  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorBookmarkBarForeground] = {kColorTabForegroundActiveFrameActive};
  mixer[kColorDownloadShelfButtonText] = {SK_ColorWHITE};
  mixer[kColorForTest] = {SkColorSetRGB(0x00, 0x00, 0x00)};
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

  AddBraveFeaturesColors(key, &mixer, ProfileType::kNormalProfile);
}

void AddBravePrivateThemeColorMixer(ui::ColorProvider* provider,
                                    const ui::ColorProviderManager::Key& key) {
  AddBraveDarkThemeColorMixer(provider, key);

  ui::ColorMixer& mixer = provider->AddMixer();

  mixer[kColorBookmarkBarForeground] = {SkColorSetRGB(0xFF, 0xFF, 0xFF)};
  mixer[kColorForTest] = {SkColorSetRGB(0xFF, 0x00, 0x00)};
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

  AddBraveFeaturesColors(key, &mixer, ProfileType::kPrivateProfile);
}

void AddBraveTorThemeColorMixer(ui::ColorProvider* provider,
                                const ui::ColorProviderManager::Key& key) {
  AddBravePrivateThemeColorMixer(provider, key);

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

  AddBraveFeaturesColors(key, &mixer, ProfileType::kTorProfile);
}

void AddBraveOmniboxLightThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key) {
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

void AddBraveOmniboxDarkThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key) {
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

void AddBraveOmniboxPrivateThemeColorMixer(
    ui::ColorProvider* provider,
    const ui::ColorProviderManager::Key& key) {
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
