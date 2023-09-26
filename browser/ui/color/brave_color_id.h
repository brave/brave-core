/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_ID_H_
#define BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_ID_H_

#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "chrome/browser/ui/color/chrome_color_id.h"

// clang-format off

#define BRAVE_COMMON_COLOR_IDS                  \
    E_CPONLY(kColorForTest)                     \
    E_CPONLY(kColorIconBase)                    \
    E_CPONLY(kColorMenuItemSubText)             \
    E_CPONLY(kColorBookmarkBarInstructionsText) \
    E_CPONLY(kColorLocationBarFocusRing)        \
    E_CPONLY(kColorDialogDontAskAgainButton)    \
    E_CPONLY(kColorDialogDontAskAgainButtonHovered) \
    E_CPONLY(kColorWebDiscoveryInfoBarBackground)   \
    E_CPONLY(kColorWebDiscoveryInfoBarMessage)      \
    E_CPONLY(kColorWebDiscoveryInfoBarLink)         \
    E_CPONLY(kColorWebDiscoveryInfoBarNoThanks)     \
    E_CPONLY(kColorWebDiscoveryInfoBarClose)        \
    E_CPONLY(kColorBraveDownloadToolbarButtonActive)

#define BRAVE_SEARCH_CONVERSION_COLOR_IDS                             \
    E_CPONLY(kColorSearchConversionCloseButton)                       \
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
    E_CPONLY(kColorSidebarButtonPressed)                      \
    E_CPONLY(kColorSidebarItemBackgroundHovered)              \
    E_CPONLY(kColorSidebarItemDragIndicator)                  \
    E_CPONLY(kColorSidebarSeparator)                          \
    E_CPONLY(kColorSidebarPanelHeaderSeparator)               \
    E_CPONLY(kColorSidebarPanelHeaderBackground)              \
    E_CPONLY(kColorSidebarPanelHeaderTitle)                   \
    E_CPONLY(kColorSidebarPanelHeaderButton)                  \
    E_CPONLY(kColorSidebarPanelHeaderButtonHovered)

#if BUILDFLAG(ENABLE_SPEEDREADER)
#define BRAVE_SPEEDREADER_COLOR_IDS      \
  E_CPONLY(kColorSpeedreaderIcon)        \
  E_CPONLY(kColorSpeedreaderToggleThumb) \
  E_CPONLY(kColorSpeedreaderToggleTrack) \
  E_CPONLY(kColorSpeedreaderToolbarBackground) \
  E_CPONLY(kColorSpeedreaderToolbarBorder) \
  E_CPONLY(kColorSpeedreaderToolbarForeground) \
  E_CPONLY(kColorSpeedreaderToolbarButtonHover) \
  E_CPONLY(kColorSpeedreaderToolbarButtonActive) \
  E_CPONLY(kColorSpeedreaderToolbarButtonBorder)
#else
#define BRAVE_SPEEDREADER_COLOR_IDS
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#define BRAVE_VPN_COLOR_IDS                        \
    E_CPONLY(kColorBraveVpnButtonErrorBorder)      \
    E_CPONLY(kColorBraveVpnButtonBorder)           \
    E_CPONLY(kColorBraveVpnButtonText)    \
    E_CPONLY(kColorBraveVpnButtonTextError)    \
    E_CPONLY(kColorBraveVpnButtonIconConnected)    \
    E_CPONLY(kColorBraveVpnButtonIconDisconnected) \
    E_CPONLY(kColorBraveVpnButtonIconError) \
    E_CPONLY(kColorBraveVpnButtonBackgroundNormal) \
    E_CPONLY(kColorBraveVpnButtonBackgroundHover)  \
    E_CPONLY(kColorBraveVpnButtonErrorBackgroundNormal) \
    E_CPONLY(kColorBraveVpnButtonErrorBackgroundHover)  \
    E_CPONLY(kColorBraveVpnButtonIconInner) \
    E_CPONLY(kColorBraveVpnButtonIconErrorInner)
#else
#define BRAVE_VPN_COLOR_IDS
#endif

// Unfortunately, we can't have a defined(TOOLKIT_VIEWS) guard here
// as brave_color_mixer depends on this without deps to //ui/views:flags.
// But it's safe have without the guard as this file is included only when
// !is_android.
#define BRAVE_VERTICAL_TAB_COLOR_IDS                    \
    E_CPONLY(kColorBraveVerticalTabSeparator)           \
    E_CPONLY(kColorBraveVerticalTabActiveBackground)    \
    E_CPONLY(kColorBraveVerticalTabInactiveBackground)  \
    E_CPONLY(kColorBraveVerticalTabHeaderButtonColor)   \
    E_CPONLY(kColorBraveVerticalTabNTBIconColor)        \
    E_CPONLY(kColorBraveVerticalTabNTBTextColor)        \
    E_CPONLY(kColorBraveVerticalTabNTBShortcutTextColor)

#define BRAVE_PLAYLIST_COLOR_IDS                                      \
    E_CPONLY(kColorBravePlaylistAddedIcon)                            \
    E_CPONLY(kColorBravePlaylistCheckedIcon)                          \
    E_CPONLY(kColorBravePlaylistSelectedBackground)                   \
    E_CPONLY(kColorBravePlaylistListBorder)                           \
    E_CPONLY(kColorBravePlaylistMoveDialogDescription)                \
    E_CPONLY(kColorBravePlaylistMoveDialogCreatePlaylistAndMoveTitle) \
    E_CPONLY(kColorBravePlaylistNewPlaylistDialogNameLabel)           \
    E_CPONLY(kColorBravePlaylistNewPlaylistDialogItemsLabel)

#define BRAVE_OMNIBOX_COLOR_IDS \
    E_CPONLY(kColorBraveOmniboxResultViewSeparator)

#define BRAVE_COLOR_IDS               \
    BRAVE_COMMON_COLOR_IDS            \
    BRAVE_SEARCH_CONVERSION_COLOR_IDS \
    BRAVE_SIDEBAR_COLOR_IDS           \
    BRAVE_SPEEDREADER_COLOR_IDS       \
    BRAVE_VPN_COLOR_IDS               \
    BRAVE_VERTICAL_TAB_COLOR_IDS      \
    BRAVE_PLAYLIST_COLOR_IDS          \
    BRAVE_OMNIBOX_COLOR_IDS

#include "ui/color/color_id_macros.inc"

enum BraveColorIds : ui::ColorId {
  kBraveColorsStart = kChromeColorsEnd,

  BRAVE_COLOR_IDS

  kBraveColorsEnd,
};

#include "ui/color/color_id_macros.inc"  // NOLINT

// clang-format on

#endif  // BRAVE_BROWSER_UI_COLOR_BRAVE_COLOR_ID_H_
