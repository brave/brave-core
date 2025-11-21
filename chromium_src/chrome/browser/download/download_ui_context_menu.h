/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_UI_CONTEXT_MENU_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_UI_CONTEXT_MENU_H_

#include "brave/browser/download/brave_download_commands.h"
#include "chrome/browser/download/download_commands.h"
#include "chrome/browser/download/download_item_model.h"

// Scrubs out code for histogram.
#define RecordCommandsEnabled(...)      \
  RecordCommandsEnabled(__VA_ARGS__) {} \
  void RecordCommandsEnabled_Unused(__VA_ARGS__)

// Add Brave's unittest as friend.
#define DetachFromDownloadItem() \
  DetachFromDownloadItem();      \
  friend class DownloadBubbleTest

// Add a decorator around upstream's GetMenuModel to insert Brave-specific
// commands.
#define GetMenuModel       \
  GetMenuModel_Chromium(); \
  ui::SimpleMenuModel* GetMenuModel

// Replace DownloadCommands with BraveDownloadCommands in the member variable.
// BraveDownloadCommands adds additional commands specific to Brave, such as
// "Delete Local File" and "Copy Download Link".
#define DownloadCommands BraveDownloadCommands

#include <chrome/browser/download/download_ui_context_menu.h>  // IWYU pragma: export

#undef DownloadCommands
#undef GetMenuModel
#undef DetachFromDownloadItem
#undef RecordCommandsEnabled

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_UI_CONTEXT_MENU_H_
