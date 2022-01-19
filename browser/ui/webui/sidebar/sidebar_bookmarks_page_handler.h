/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SIDEBAR_SIDEBAR_BOOKMARKS_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SIDEBAR_SIDEBAR_BOOKMARKS_PAGE_HANDLER_H_

#include <string>

#include "brave/browser/ui/webui/sidebar/sidebar.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

class GURL;

class SidebarBookmarksPageHandler
    : public sidebar::mojom::BookmarksPageHandler {
 public:
  explicit SidebarBookmarksPageHandler(
      mojo::PendingReceiver<sidebar::mojom::BookmarksPageHandler> receiver);
  ~SidebarBookmarksPageHandler() override;
  SidebarBookmarksPageHandler(const SidebarBookmarksPageHandler&) = delete;
  SidebarBookmarksPageHandler& operator=(const SidebarBookmarksPageHandler&) =
      delete;

 private:
  // sidebar::mojom::BookmarksPageHandler:
  void OpenBookmark(const GURL& url,
                    int32_t parent_folder_depth,
                    ui::mojom::ClickModifiersPtr click_modifiers) override;
  void ShowContextMenu(const std::string& id, const gfx::Point& point) override;

  mojo::Receiver<sidebar::mojom::BookmarksPageHandler> receiver_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SIDEBAR_SIDEBAR_BOOKMARKS_PAGE_HANDLER_H_
