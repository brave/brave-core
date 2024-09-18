/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/commander/commander_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/grit/generated_resources.h"
#include "components/dom_distiller/core/dom_distiller_features.h"
#include "components/grit/brave_components_strings.h"

namespace {
constexpr int kPasteMacResourceId = IDS_PASTE_MAC;
constexpr int kMuteSiteResourceId = IDS_MUTE_SITE_MAC;
constexpr int kCloseOtherTabsResourceId = IDS_TAB_CXMENU_CLOSEOTHERTABS;
constexpr int kReopenCloseTabsMacResourceId = IDS_REOPEN_CLOSED_TABS_MAC;
constexpr int kHelpMacResourceId = IDS_HELP_MAC;
constexpr int kMediaRouterMenuItemTitleResourceId =
    IDS_MEDIA_ROUTER_MENU_ITEM_TITLE;
constexpr int kShowGoogleLensShortcut = IDC_SHOW_GOOGLE_LENS_SHORTCUT;
}  // namespace

// Insert "New Private Window with Tor" in "File" menu
#undef IDS_REOPEN_CLOSED_TABS_MAC
#define IDS_REOPEN_CLOSED_TABS_MAC                  \
  IDS_NEW_OFFTHERECORD_WINDOW_TOR)                  \
      .command_id(IDC_NEW_OFFTHERECORD_WINDOW_TOR), \
  Item(kReopenCloseTabsMacResourceId

// Insert "Report Broken Site" in "Help" menu
#undef IDS_HELP_MAC
#define IDS_HELP_MAC                                    \
  IDS_REPORT_BROKEN_SITE_MAC)                           \
        .command_id(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER), \
  Item(kHelpMacResourceId

#undef IDS_PASTE_MAC
#define IDS_PASTE_MAC IDS_COPY_CLEAN_LINK) \
                    .command_id(IDC_COPY_CLEAN_LINK) \
                    .target(app_delegate), \
        Item(kPasteMacResourceId

#undef IDS_MUTE_SITE_MAC
#define IDS_MUTE_SITE_MAC                          \
IDS_MUTE_TAB_MAC).command_id(IDC_TOGGLE_TAB_MUTE), \
              Item(kMuteSiteResourceId

#undef IDS_TAB_CXMENU_CLOSEOTHERTABS
#define IDS_TAB_CXMENU_CLOSEOTHERTABS                                      \
IDS_TAB_CXMENU_CLOSE_DUPLICATE_TABS).command_id(IDC_CLOSE_DUPLICATE_TABS), \
              Item(kCloseOtherTabsResourceId

// Insert Commander item right after media router
#undef IDS_MEDIA_ROUTER_MENU_ITEM_TITLE
#define IDS_MEDIA_ROUTER_MENU_ITEM_TITLE                         \
IDS_IDC_COMMANDER).command_id(IDC_COMMANDER)                     \
                  .remove_if(is_pwa || !commander::IsEnabled()), \
              Item(kMediaRouterMenuItemTitleResourceId

// Remove "Always Show Google Lens Shortcut"
#undef IDC_SHOW_GOOGLE_LENS_SHORTCUT
#define IDC_SHOW_GOOGLE_LENS_SHORTCUT \
  kShowGoogleLensShortcut).remove_if(true

#include "src/chrome/browser/ui/cocoa/main_menu_builder.mm"

#undef IDC_SHOW_GOOGLE_LENS_SHORTCUT
#undef IDS_MEDIA_ROUTER_MENU_ITEM_TITLE
#undef IDS_MUTE_SITE_MAC
#define IDS_MUTE_SITE_MAC kMuteSiteResourceId
#undef IDS_PASTE_MAC
#define IDS_PASTE_MAC kPasteMacResourceId
#undef IDS_TAB_CXMENU_CLOSEOTHERTABS
#define IDS_TAB_CXMENU_CLOSEOTHERTABS kCloseOtherTabsResourceId
#undef IDS_HELP_MAC
#define IDS_HELP_MAC kHelpMacResourceId
#undef IDS_REOPEN_CLOSED_TABS_MAC
#define IDS_REOPEN_CLOSED_TABS_MAC kReopenCloseTabsMacResourceId
