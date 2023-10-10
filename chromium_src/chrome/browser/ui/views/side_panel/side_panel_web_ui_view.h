/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_WEB_UI_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_WEB_UI_VIEW_H_

#include "ui/views/controls/webview/webview.h"

#define ViewHierarchyChanged \
  AddedToWidget() override;  \
  void ViewHierarchyChanged

#include "src/chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"  // IWYU pragma: export

#undef ViewHierarchyChanged

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_WEB_UI_VIEW_H_
