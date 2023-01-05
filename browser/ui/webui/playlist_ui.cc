/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/playlist_ui.h"

#include <utility>

#include "base/feature_list.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/playlist/features.h"
#include "brave/components/playlist/playlist_service.h"
#include "brave/components/playlist/resources/grit/playlist_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/grit/brave_components_resources.h"
#include "components/sessions/content/session_tab_helper.h"
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
  if (page_handler_factory_receiver_.is_bound())
    page_handler_factory_receiver_.reset();

  page_handler_factory_receiver_.Bind(std::move(pending_receiver));
}

void PlaylistUI::CreatePageHandler(
    mojo::PendingRemote<playlist::mojom::PlaylistServiceObserver>
        service_observer,
    mojo::PendingReceiver<playlist::mojom::PlaylistService> pending_service) {
  DCHECK(service_observer.is_valid());

  auto* service = playlist::PlaylistServiceFactory::GetForBrowserContext(
      Profile::FromWebUI(web_ui()));
  service_receivers_.Add(service, std::move(pending_service));
  service->AddObserver(std::move(service_observer));

  // When WebUI calls this, mark that the page can be shown on sidebar.
  if (embedder_)
    embedder_->ShowUI();
}

void PlaylistUI::GetActiveTabId(GetActiveTabIdCallback callback) {
  auto* browser = chrome::FindBrowserWithProfile(Profile::FromWebUI(web_ui()));
  constexpr auto kInvalidSessionId = SessionID::InvalidValue();
  if (!browser) {
    std::move(callback).Run(kInvalidSessionId.id(), kInvalidSessionId.id());
    return;
  }

  auto* tab_strip_model = browser->tab_strip_model();
  DCHECK(tab_strip_model);

  auto active_index = tab_strip_model->active_index();
  if (active_index == TabStripModel::kNoTab) {
    std::move(callback).Run(kInvalidSessionId.id(), kInvalidSessionId.id());
    return;
  }

  auto* active_contents = tab_strip_model->GetWebContentsAt(active_index);
  DCHECK(active_contents);
  std::move(callback).Run(
      sessions::SessionTabHelper::IdForWindowContainingTab(active_contents)
          .id(),
      sessions::SessionTabHelper::IdForTab(active_contents).id());
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
