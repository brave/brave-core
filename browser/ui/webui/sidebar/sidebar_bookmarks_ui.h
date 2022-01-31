// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SIDEBAR_SIDEBAR_BOOKMARKS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_SIDEBAR_SIDEBAR_BOOKMARKS_UI_H_

#include <memory>

#include "brave/browser/ui/webui/sidebar/sidebar.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

class SidebarBookmarksPageHandler;

class SidebarBookmarksUI : public ui::MojoWebUIController,
                           public sidebar::mojom::BookmarksPageHandlerFactory {
 public:
  explicit SidebarBookmarksUI(content::WebUI* web_ui);
  ~SidebarBookmarksUI() override;
  SidebarBookmarksUI(const SidebarBookmarksUI&) = delete;
  SidebarBookmarksUI& operator=(const SidebarBookmarksUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<sidebar::mojom::BookmarksPageHandlerFactory>
          receiver);

 private:
  // sidebar::mojom::BookmarksPageHandlerFactory
  void CreateBookmarksPageHandler(
      mojo::PendingReceiver<sidebar::mojom::BookmarksPageHandler> receiver)
      override;

  std::unique_ptr<SidebarBookmarksPageHandler> bookmarks_page_handler_;
  mojo::Receiver<sidebar::mojom::BookmarksPageHandlerFactory>
      bookmarks_page_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SIDEBAR_SIDEBAR_BOOKMARKS_UI_H_
