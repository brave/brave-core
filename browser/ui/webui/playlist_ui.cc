/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
#include "chrome/browser/profiles/profile.h"
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/playlist_ui.h"

#include <utility>

#include "base/feature_list.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/playlist_page_handler.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/playlist/features.h"
#include "brave/components/playlist/resources/grit/playlist_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/bindings_policy.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

namespace playlist {

// static
bool PlaylistUI::ShouldBlockPlaylistWebUI(
    content::BrowserContext* browser_context,
    const GURL& url) {
  if (url.host_piece() != kPlaylistHost)
    return false;

  return !PlaylistServiceFactory::IsPlaylistEnabled(browser_context);
}

PlaylistUI::PlaylistUI(content::WebUI* web_ui, const std::string& name)
    : UntrustedWebUIController(web_ui) {
  // From MojoWebUIController
  web_ui->SetBindings(content::BINDINGS_POLICY_MOJO_WEB_UI);

  auto* source =
      CreateAndAddWebUIDataSource(web_ui, name, kPlaylistGenerated,
                                  kPlaylistGeneratedSize, IDR_PLAYLIST_HTML);

  // Allow to load untrusted resources.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      std::string("style-src chrome-untrusted://resources "
                  "'unsafe-inline';"));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      std::string("script-src 'self' "
                  "chrome-untrusted://resources;"));

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      std::string("img-src 'self' chrome-untrusted://playlist-data;"));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::MediaSrc,
      std::string("media-src 'self' chrome-untrusted://playlist-data;"));
}

PlaylistUI::~PlaylistUI() = default;

void PlaylistUI::BindInterface(
    mojo::PendingReceiver<playlist::mojom::PageHandlerFactory>
        pending_receiver) {
  if (page_factory_receiver_.is_bound())
    page_factory_receiver_.reset();

  page_factory_receiver_.Bind(std::move(pending_receiver));
}

void PlaylistUI::CreatePageHandler(
    mojo::PendingRemote<playlist::mojom::Page> pending_page,
    mojo::PendingReceiver<playlist::mojom::PageHandler> pending_page_handler) {
  DCHECK(pending_page.is_valid());
  page_handler_ = std::make_unique<PlaylistPageHandler>(
      Profile::FromWebUI(web_ui()), web_ui()->GetWebContents(),
      std::move(pending_page_handler), std::move(pending_page));

  // When WebUI calls this, mark that the page can be shown on sidebar.
  if (embedder_)
    embedder_->ShowUI();
}

WEB_UI_CONTROLLER_TYPE_IMPL(PlaylistUI)

std::unique_ptr<content::WebUIController>
UntrustedPlaylistUIConfig::CreateWebUIController(content::WebUI* web_ui) {
  return std::make_unique<PlaylistUI>(web_ui, kPlaylistURL);
}

bool UntrustedPlaylistUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return base::FeatureList::IsEnabled(playlist::features::kPlaylist);
}

UntrustedPlaylistUIConfig::UntrustedPlaylistUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kPlaylistHost) {}

}  // namespace playlist
