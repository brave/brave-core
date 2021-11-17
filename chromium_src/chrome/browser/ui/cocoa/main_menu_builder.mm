/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_command_ids.h"
#include "brave/grit/brave_generated_resources.h"

#define BRAVE_BUILD_FILE_MENU                       \
  Item(IDS_NEW_OFFTHERECORD_WINDOW_TOR)             \
      .command_id(IDC_NEW_OFFTHERECORD_WINDOW_TOR),

#define BRAVE_BUILD_HELP_MENU                         \
  Item(IDS_REPORT_BROKEN_SITE_MAC)                    \
      .command_id(IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER),

#include "src/chrome/browser/ui/cocoa/main_menu_builder.mm"
#undef BRAVE_WEBCOMPAT_REPORTER_MENU_ENTRY
