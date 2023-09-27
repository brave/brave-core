/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/playlist_ui.h"

#include <utility>

#include "base/feature_list.h"
#include "base/strings/strcat.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/ui/playlist/playlist_dialogs.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/browser/resources/grit/playlist_generated_map.h"
#include "brave/components/playlist/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_resources.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/bindings_policy.h"
#include "content/public/common/url_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "url/gurl.h"

namespace playlist {

void AddLocalizedStrings(content::WebUIDataSource* source) {
  static constexpr webui::LocalizedString kLocalizedStrings[] = {
      {"braveDefaultPlaylistName", IDS_PLAYLIST_DEFAULT_PLAYLIST_NAME},
      {"bravePlaylistContextMenuEdit", IDS_PLAYLIST_CONTEXT_MENU_EDIT},
      {"bravePlaylistContextMenuShare", IDS_PLAYLIST_CONTEXT_MENU_SHARE},
      {"bravePlaylistContextMenuKeepForOfflinePlaying",
       IDS_PLAYLIST_CONTEXT_MENU_KEEP_FOR_OFFLINE_PLAYING},
      {"bravePlaylistContextMenuRemovePlayedContents",
       IDS_PLAYLIST_CONTEXT_MENU_REMOVE_PLAYED_CONTENTS},
      {"bravePlaylistContextMenuMove", IDS_PLAYLIST_CONTEXT_MENU_MOVE},
      {"bravePlaylistContextMenuRemoveOfflineData",
       IDS_PLAYLIST_CONTEXT_MENU_REMOVE_OFFLINE_DATA},
      {"bravePlaylistContextMenuRemoveFromPlaylist",
       IDS_PLAYLIST_CONTEXT_MENU_REMOVE_FROM_PLAYLIST},
      {"bravePlaylistContextMenuRenamePlaylist",
       IDS_PLAYLIST_CONTEXT_MENU_RENAME_PLAYLIST},
      {"bravePlaylistContextMenuDeletePlaylist",
       IDS_PLAYLIST_CONTEXT_MENU_DELETE_PLAYLIST},
      {"bravePlaylistContextMenuViewOriginalPage",
       IDS_PLAYLIST_CONTEXT_MENU_VIEW_ORIGINAL_PAGE},
      {"bravePlaylistEmptyFolderMessage", IDS_PLAYLIST_EMPTY_FOLDER_MESSAGE},
  };

  for (const auto& [name, id] : kLocalizedStrings) {
    source->AddString(name, l10n_util::GetStringUTF16(id));
  }
}

namespace {

class UntrustedPlayerUI : public ui::UntrustedWebUIController {
 public:
  explicit UntrustedPlayerUI(content::WebUI* web_ui)
      : UntrustedWebUIController(web_ui) {
    auto* source = CreateAndAddWebUIDataSource(
        web_ui, kPlaylistPlayerURL, kPlaylistGenerated, kPlaylistGeneratedSize,
        IDR_PLAYLIST_PLAYER_HTML);
    source->AddFrameAncestor(GURL(kPlaylistURL));
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::ScriptSrc,
        std::string("script-src 'self' chrome-untrusted://resources "
                    "chrome-untrusted://brave-resources;"));
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::MediaSrc,
        std::string("media-src 'self' chrome-untrusted://playlist-data "
                    "https://*.googlevideo.com:*;"));
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::StyleSrc,
        std::string("style-src chrome-untrusted://resources "
                    "chrome-untrusted://brave-resources 'unsafe-inline';"));
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::ImgSrc,
        std::string("img-src 'self' chrome-untrusted://playlist-data "
                    "chrome-untrusted://resources;"));
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::FontSrc,
        std::string("font-src 'self' chrome-untrusted://resources;"));
  }

  UntrustedPlayerUI(const UntrustedPlayerUI&) = delete;
  UntrustedPlayerUI& operator=(const UntrustedPlayerUI&) = delete;
  ~UntrustedPlayerUI() override = default;
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// PlaylistUI
//
// static
bool PlaylistUI::ShouldBlockPlaylistWebUI(
    content::BrowserContext* browser_context,
    const GURL& url) {
  if (url.host_piece() != kPlaylistHost) {
    return false;
  }

  return !PlaylistServiceFactory::IsPlaylistEnabled(browser_context);
}

PlaylistUI::PlaylistUI(content::WebUI* web_ui, const std::string& name)
    : UntrustedWebUIController(web_ui) {
  // From MojoWebUIController
  web_ui->SetBindings(content::BINDINGS_POLICY_MOJO_WEB_UI);

  auto* source =
      CreateAndAddWebUIDataSource(web_ui, name, kPlaylistGenerated,
                                  kPlaylistGeneratedSize, IDR_PLAYLIST_HTML);

  AddLocalizedStrings(source);

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
      std::string("img-src 'self' chrome-untrusted://playlist-data "
                  "chrome-untrusted://resources;"));
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      std::string("font-src 'self' chrome-untrusted://resources;"));

  web_ui->AddRequestableScheme(content::kChromeUIUntrustedScheme);
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      base::StrCat({"frame-src ", kPlaylistPlayerURL, ";"}));
}

PlaylistUI::~PlaylistUI() = default;

void PlaylistUI::BindInterface(
    mojo::PendingReceiver<playlist::mojom::PageHandlerFactory>
        pending_receiver) {
  if (page_handler_factory_receiver_.is_bound()) {
    page_handler_factory_receiver_.reset();
  }

  page_handler_factory_receiver_.Bind(std::move(pending_receiver));
}

void PlaylistUI::CreatePageHandler(
    mojo::PendingRemote<playlist::mojom::PlaylistServiceObserver>
        service_observer,
    mojo::PendingReceiver<playlist::mojom::PlaylistService> pending_service,
    mojo::PendingReceiver<playlist::mojom::PlaylistNativeUI> native_ui) {
  DCHECK(service_observer.is_valid());

  auto* service = playlist::PlaylistServiceFactory::GetForBrowserContext(
      Profile::FromWebUI(web_ui()));
  native_ui_receivers_.Add(this, std::move(native_ui));
  service_receivers_.Add(service, std::move(pending_service));
  service->AddObserver(std::move(service_observer));

  // When WebUI calls this, mark that the page can be shown on sidebar.
  if (embedder_) {
    embedder_->ShowUI();
  }
}

void PlaylistUI::ShowCreatePlaylistUI() {
  playlist::ShowCreatePlaylistDialog(web_ui()->GetWebContents());
}

void PlaylistUI::ShowRemovePlaylistUI(const std::string& playlist_id) {
  playlist::ShowRemovePlaylistDialog(web_ui()->GetWebContents(), playlist_id);
}

void PlaylistUI::ShowMoveItemsUI(const std::string& playlist_id,
                                 const std::vector<std::string>& items) {
  playlist::ShowMoveItemsDialog(web_ui()->GetWebContents(), playlist_id, items);
}

WEB_UI_CONTROLLER_TYPE_IMPL(PlaylistUI)

////////////////////////////////////////////////////////////////////////////////
// UntrustedPlaylistUIConfig
//
std::unique_ptr<content::WebUIController>
UntrustedPlaylistUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                 const GURL& url) {
  return std::make_unique<PlaylistUI>(web_ui, kPlaylistURL);
}

bool UntrustedPlaylistUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return base::FeatureList::IsEnabled(playlist::features::kPlaylist);
}

UntrustedPlaylistUIConfig::UntrustedPlaylistUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kPlaylistHost) {}

////////////////////////////////////////////////////////////////////////////////
// UntrustedPlaylistPlayerUIConfig
//
UntrustedPlaylistPlayerUIConfig::UntrustedPlaylistPlayerUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kPlaylistPlayerHost) {}

std::unique_ptr<content::WebUIController>
UntrustedPlaylistPlayerUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                                       const GURL& url) {
  return std::make_unique<UntrustedPlayerUI>(web_ui);
}

}  // namespace playlist
