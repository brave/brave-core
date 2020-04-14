/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_APP_BRAVE_COMMAND_IDS_H_
#define BRAVE_APP_BRAVE_COMMAND_IDS_H_

// First brave id must be higher than last chrome command.
// Check chrome/app/chrome_command_ids.h when rebase.
// ID of IDC_BRAVE_COMANDS_START and first brave command should be same.
#define IDC_BRAVE_COMMANDS_START 56000
#define IDC_SHOW_BRAVE_REWARDS   56000
#define IDC_SHOW_BRAVE_ADBLOCK   56001
#define IDC_NEW_TOR_CONNECTION_FOR_SITE     56002
#define IDC_NEW_OFFTHERECORD_WINDOW_TOR 56003
#define IDC_CONTENT_CONTEXT_OPENLINKTOR 56004
#define IDC_SHOW_BRAVE_SYNC      56005
#define IDC_SHOW_BRAVE_WALLET    56006
#define IDC_ADD_NEW_PROFILE      56007
#define IDC_OPEN_GUEST_PROFILE   56008
#define IDC_SHOW_BRAVE_WEBCOMPAT_REPORTER   56009
#define IDC_TOGGLE_SPEEDREADER   56010

#define IDC_BRAVE_COMMANDS_LAST  57000

#endif  // BRAVE_APP_BRAVE_COMMAND_IDS_H_
