/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_SHELF_CONTEXT_MENU_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_SHELF_CONTEXT_MENU_VIEW_H_

#define DownloadShelfContextMenuView DownloadShelfContextMenuViewChromium
// To make private methods accessible from our subclass.
#define OnMenuClosed \
  UnUsed() {}        \
                     \
 protected:          \
  void OnMenuClosed

#include "src/chrome/browser/ui/views/download/download_shelf_context_menu_view.h"  // IWYU pragma: export

#undef OnMenuClosed
#undef DownloadShelfContextMenuView

class DownloadShelfContextMenuView
    : public DownloadShelfContextMenuViewChromium {
 public:
  using DownloadShelfContextMenuViewChromium::
      DownloadShelfContextMenuViewChromium;
  ~DownloadShelfContextMenuView() override;

  // DownloadShelfContextMenuViewChromium overrides:
  ui::SimpleMenuModel* GetMenuModel() override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdVisible(int command_id) const override;
  void ExecuteCommand(int command_id, int event_flags) override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_SHELF_CONTEXT_MENU_VIEW_H_
