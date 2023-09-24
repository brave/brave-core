// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/toolbar/app_menu_icons.h"

#include <map>

#include "base/no_destructor.h"
#include "brave/app/brave_command_ids.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/app/chrome_command_ids.h"
#include "ui/base/models/image_model.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/color/color_id.h"
#include "ui/gfx/vector_icon_types.h"

namespace {

constexpr int kLeoIconSize = 16;
const std::map<int, const gfx::VectorIcon&>& GetCommandIcons() {
  static const base::NoDestructor<std::map<int, const gfx::VectorIcon&>>
      kCommandIcons({
          // The `App` menu
          {IDC_NEW_TAB, kLeoBrowserAddIcon},
          {IDC_NEW_WINDOW, kLeoWindowTabNewIcon},
          {IDC_NEW_INCOGNITO_WINDOW, kLeoWindowTabPrivateIcon},
          {IDC_NEW_TOR_CONNECTION_FOR_SITE, kLeoWindowTabTorIcon},
          {IDC_NEW_OFFTHERECORD_WINDOW_TOR, kLeoWindowTabTorIcon},
          {IDC_SHOW_BRAVE_REWARDS, kLeoProductBatOutlineIcon},
          {IDC_SHOW_BRAVE_WALLET, kLeoProductBraveWalletIcon},
          {IDC_BRAVE_VPN_MENU, kLeoProductVpnIcon},
          {IDC_APP_MENU_IPFS, kLeoProductIpfsOutlineIcon},
          {IDC_RECENT_TABS_MENU, kLeoHistoryIcon},
          {IDC_BOOKMARKS_MENU, kLeoProductBookmarksIcon},
          {IDC_VIEW_PASSWORDS, kLeoKeyIcon},
          {IDC_SHOW_DOWNLOADS, kLeoDownloadIcon},
          {IDC_MANAGE_EXTENSIONS, kLeoBrowserExtensionsIcon},
          {IDC_ZOOM_MENU, kLeoSearchZoomInIcon},
          {IDC_PRINT, kLeoPrintIcon},
          {IDC_MORE_TOOLS_MENU, kLeoWindowScrewdriverIcon},
          {IDC_EDIT_MENU, kLeoCopyPlainTextIcon},
          {IDC_OPTIONS, kLeoSettingsIcon},
          {IDC_HELP_MENU, kLeoHelpOutlineIcon},
          {IDC_EXIT, kLeoCloseIcon},

          // The `VPN` submenu
          {IDC_TOGGLE_BRAVE_VPN, kLeoProductVpnIcon},
          {IDC_TOGGLE_BRAVE_VPN_TOOLBAR_BUTTON, kLeoEyeOnIcon},
          {IDC_SHOW_BRAVE_VPN_PANEL, kLeoProductVpnIcon},
          {IDC_SEND_BRAVE_VPN_FEEDBACK, kLeoMessageHeartIcon},
          {IDC_MANAGE_BRAVE_VPN_PLAN, kLeoLaunchIcon},
          {IDC_ABOUT_BRAVE_VPN, kLeoInfoOutlineIcon},

          // The `Bookmarks` submenu
          {IDC_BOOKMARK_THIS_TAB, kLeoBrowserBookmarkAddIcon},
          {IDC_BOOKMARK_ALL_TABS, kLeoBrowserBookmarkPluralIcon},
          {IDC_BRAVE_BOOKMARK_BAR_SUBMENU, kLeoProductBookmarksIcon},
          {IDC_SHOW_BOOKMARK_MANAGER, kLeoWindowBookmarkIcon},
          {IDC_IMPORT_SETTINGS, kLeoImportArrowIcon},
          {IDC_READING_LIST_MENU, kLeoReadingListIcon},

          // The `History submenu
          {IDC_SHOW_HISTORY, kLeoHistoryIcon},
          {IDC_CLEAR_BROWSING_DATA, kLeoShredDataIcon},
          {IDC_RECENT_TABS_NO_DEVICE_TABS, kLeoSmartphoneLaptopIcon},

          // The `Help` menu
          {IDC_ABOUT, kLeoBraveIconMonochromeIcon},
          {IDC_HELP_PAGE_VIA_MENU, kLeoHelpOutlineIcon},
          {IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER, kLeoWarningTriangleOutlineIcon},

          // The `More tools` menu
          {IDC_ADD_NEW_PROFILE, kLeoUserAddIcon},
          {IDC_OPEN_GUEST_PROFILE, kLeoUserCircleIcon},
          {IDC_FIND, kLeoSearchIcon},
          {IDC_SAVE_PAGE, kLeoFileDownloadIcon},
          {IDC_CREATE_SHORTCUT, kLeoShareWindowsIcon},
          {IDC_NAME_WINDOW, kLeoEditBoxIcon},
          {IDC_TOGGLE_QUICK_COMMANDS, kLeoArrowSmallRightIcon},
          {IDC_PERFORMANCE, kLeoNetworkSpeedFastIcon},
          {IDC_DEV_TOOLS, kLeoCodeIcon},
          {IDC_TASK_MANAGER, kLeoWindowBinaryCodeIcon},
          {IDC_SHOW_BRAVE_SYNC, kLeoProductSyncIcon},
          {IDC_ROUTE_MEDIA, kLeoChromeCastIcon},
          {IDC_INSTALL_PWA, kLeoPwaInstallIcon},
          {IDC_OPEN_IN_PWA_WINDOW, kLeoLaunchIcon},
          {IDC_SIDEBAR_SHOW_OPTION_MENU, kLeoBrowserSidebarRightIcon},
      });
  return *kCommandIcons.get();
}

}  // namespace

void ApplyLeoIcons(ui::SimpleMenuModel* menu) {
  // Menu can be null in some tests.
  if (!menu) {
    return;
  }

  auto command_icons = GetCommandIcons();

  for (size_t i = 0; i < menu->GetItemCount(); ++i) {
    auto command = menu->GetCommandIdAt(i);
    const auto& it = command_icons.find(command);
    if (it == command_icons.end()) {
      continue;
    }

    const auto& icon = it->second;
    menu->SetIcon(i, ui::ImageModel::FromVectorIcon(
                         icon, ui::ColorIds::kColorMenuIcon, kLeoIconSize));
  }
}
