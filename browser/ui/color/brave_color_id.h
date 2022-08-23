/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_ID_H_
#define BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_ID_H_

#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags.h"
#include "chrome/browser/ui/color/chrome_color_id.h"

// clang-format off

#define BRAVE_COMMON_COLOR_IDS                  \
    E_CPONLY(kColorForTest)                     \
    E_CPONLY(kColorIconBase)                    \
    E_CPONLY(kColorMenuItemSubText)             \
    E_CPONLY(kColorBookmarkBarInstructionsText) \
    E_CPONLY(kColorLocationBarFocusRing)

#define BRAVE_SEARCH_CONVERSION_COLOR_IDS                             \
    E_CPONLY(kColorSearchConversionBannerTypeBackgroundBorder)        \
    E_CPONLY(kColorSearchConversionBannerTypeBackgroundBorderHovered) \
    E_CPONLY(kColorSearchConversionBannerTypeBackgroundGradientFrom)  \
    E_CPONLY(kColorSearchConversionBannerTypeBackgroundGradientTo)    \
    E_CPONLY(kColorSearchConversionBannerTypeDescText)                \
    E_CPONLY(kColorSearchConversionButtonTypeBackgroundNormal)        \
    E_CPONLY(kColorSearchConversionButtonTypeBackgroundHovered)       \
    E_CPONLY(kColorSearchConversionButtonTypeDescNormal)              \
    E_CPONLY(kColorSearchConversionButtonTypeDescHovered)             \
    E_CPONLY(kColorSearchConversionButtonTypeInputAppend)

#if BUILDFLAG(ENABLE_SIDEBAR)
#define BRAVE_SIDEBAR_COLOR_IDS                               \
    E_CPONLY(kColorSidebarAddBubbleBackground)                \
    E_CPONLY(kColorSidebarAddBubbleHeaderText)                \
    E_CPONLY(kColorSidebarAddBubbleItemTextBackgroundHovered) \
    E_CPONLY(kColorSidebarAddBubbleItemTextHovered)           \
    E_CPONLY(kColorSidebarAddBubbleItemTextNormal)            \
    E_CPONLY(kColorSidebarAddButtonDisabled)                  \
    E_CPONLY(kColorSidebarArrowBackgroundHovered)             \
    E_CPONLY(kColorSidebarArrowDisabled)                      \
    E_CPONLY(kColorSidebarArrowNormal)                        \
    E_CPONLY(kColorSidebarButtonBase)                         \
    E_CPONLY(kColorSidebarItemBackgroundHovered)              \
    E_CPONLY(kColorSidebarItemDragIndicatorColor)             \
    E_CPONLY(kColorSidebarSeparator)
#else
#define BRAVE_SIDEBAR_COLOR_IDS
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#define BRAVE_SPEEDREADER_COLOR_IDS      \
  E_CPONLY(kColorSpeedreaderIcon)        \
  E_CPONLY(kColorSpeedreaderToggleThumb) \
  E_CPONLY(kColorSpeedreaderToggleTrack)
#else
#define BRAVE_SPEEDREADER_COLOR_IDS
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#define BRAVE_VPN_COLOR_IDS                        \
    E_CPONLY(kColorBraveVpnButtonBorder)           \
    E_CPONLY(kColorBraveVpnButtonTextConnected)    \
    E_CPONLY(kColorBraveVpnButtonTextDisconnected)
#else
#define BRAVE_VPN_COLOR_IDS
#endif

#define BRAVE_COLOR_IDS               \
    BRAVE_COMMON_COLOR_IDS            \
    BRAVE_SEARCH_CONVERSION_COLOR_IDS \
    BRAVE_SIDEBAR_COLOR_IDS           \
    BRAVE_SPEEDREADER_COLOR_IDS       \
    BRAVE_VPN_COLOR_IDS

#include "ui/color/color_id_macros.inc"

enum BraveColorIds : ui::ColorId {
  kBraveColorsStart = kChromeColorsEnd,

  BRAVE_COLOR_IDS

  kBraveColorsEnd,
};

#include "ui/color/color_id_macros.inc"  // NOLINT

// clang-format on

#endif  // BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_ID_H_
