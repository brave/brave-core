/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PLAYLIST_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_PLAYLIST_UI_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace content {
class BrowserContext;
}  // namespace content

class GURL;

namespace playlist {

class PlaylistUI : public ui::UntrustedWebUIController,
                   public playlist::mojom::PageHandlerFactory,
                   public playlist::mojom::PlaylistNativeUI {
 public:
  static bool ShouldBlockPlaylistWebUI(content::BrowserContext* browser_context,
                                       const GURL& url);

  PlaylistUI(content::WebUI* web_ui, const std::string& host);
  ~PlaylistUI() override;
  PlaylistUI(const PlaylistUI&) = delete;
  PlaylistUI& operator=(const PlaylistUI&) = delete;

  void BindInterface(mojo::PendingReceiver<playlist::mojom::PageHandlerFactory>
                         pending_receiver);

  // Set by BubbleContentsWrapperT. MojoBubbleWebUIController provides default
  // implementation for this but we don't use it.
  void set_embedder(
      base::WeakPtr<ui::MojoBubbleWebUIController::Embedder> embedder) {
    embedder_ = embedder;
  }

  // playlist::mojom::PageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingRemote<playlist::mojom::PlaylistServiceObserver>
          service_observer,
      mojo::PendingReceiver<playlist::mojom::PlaylistService> service,
      mojo::PendingReceiver<playlist::mojom::PlaylistNativeUI> native_ui)
      override;

  // playlist::mojom::NativeUI:
  void ShowCreatePlaylistUI() override;
  void ShowRemovePlaylistUI(const std::string& playlist_id) override;
  void ShowMoveItemsUI(const std::string& playlist_id,
                       const std::vector<std::string>& items) override;

 private:
  base::WeakPtr<ui::MojoBubbleWebUIController::Embedder> embedder_;

  mojo::ReceiverSet<playlist::mojom::PlaylistService> service_receivers_;
  mojo::ReceiverSet<playlist::mojom::PlaylistNativeUI> native_ui_receivers_;

  mojo::Receiver<playlist::mojom::PageHandlerFactory>
      page_handler_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class UntrustedPlaylistUIConfig : public content::WebUIConfig {
 public:
  UntrustedPlaylistUIConfig();
  ~UntrustedPlaylistUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

class UntrustedPlaylistPlayerUIConfig : public content::WebUIConfig {
 public:
  UntrustedPlaylistPlayerUIConfig();
  ~UntrustedPlaylistPlayerUIConfig() override = default;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_WEBUI_PLAYLIST_UI_H_
