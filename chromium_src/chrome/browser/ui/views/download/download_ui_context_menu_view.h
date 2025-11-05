// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_UI_CONTEXT_MENU_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_UI_CONTEXT_MENU_VIEW_H_

#include "chrome/browser/download/download_commands.h"

// Extend download_commands_executed_recorded_ array size to include
// Brave-specific commands.
#define kMaxValue COPY_DOWNLOAD_LINK

#include <chrome/browser/ui/views/download/download_ui_context_menu_view.h>  // IWYU pragma: export

#undef kMaxValue

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_UI_CONTEXT_MENU_VIEW_H_
