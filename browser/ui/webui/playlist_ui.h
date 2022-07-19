/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PLAYLIST_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_PLAYLIST_UI_H_

#include <memory>
#include <string>

#include "brave/components/playlist/mojom/playlist.mojom.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace content {
class BrowserContext;
}  // namespace content

class GURL;
class PlaylistPageHandler;

namespace playlist {

class PlaylistUI : public ui::MojoWebUIController,
                   public playlist::mojom::PageHandlerFactory {
 public:
  static bool ShouldBlockPlaylistWebUI(content::BrowserContext* browser_context,
                                       const GURL& url);

  PlaylistUI(content::WebUI* web_ui, const std::string& host);
  ~PlaylistUI() override;
  PlaylistUI(const PlaylistUI&) = delete;
  PlaylistUI& operator=(const PlaylistUI&) = delete;

  void BindInterface(mojo::PendingReceiver<playlist::mojom::PageHandlerFactory>
                         pending_receiver);

  // playlist::mojom::PageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingRemote<playlist::mojom::Page> pending_page,
      mojo::PendingReceiver<playlist::mojom::PageHandler> pending_page_handler)
      override;

 private:
  std::unique_ptr<PlaylistPageHandler> page_handler_;

  mojo::Receiver<playlist::mojom::PageHandlerFactory> page_factory_receiver_{
      this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_WEBUI_PLAYLIST_UI_H_
