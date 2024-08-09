/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/playlist_ui.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/playlist/playlist_dialogs.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/playlist_active_tab_tracker.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/playlist/browser/resources/grit/playlist_generated_map.h"
#include "brave/components/playlist/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/content/session_tab_helper.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/bindings_policy.h"
#include "content/public/common/url_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "url/gurl.h"

namespace playlist {

namespace {

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
      {"bravePlaylistA11YCreatePlaylistFolder",
       IDS_PLAYLIST_A11Y_CREATE_PLAYLIST_FOLDER},
      {"bravePlaylistA11YOpenPlaylistSettings",
       IDS_PLAYLIST_A11Y_OPEN_PLAYLIST_SETTINGS},
      {"bravePlaylistA11YClosePanel", IDS_SIDEBAR_PANEL_CLOSE_BUTTON_TOOLTIP},
      {"bravePlaylistA11YPlay", IDS_PLAYLIST_A11Y_PLAY},
      {"bravePlaylistA11YPause", IDS_PLAYLIST_A11Y_PAUSE},
      {"bravePlaylistA11YNext", IDS_PLAYLIST_A11Y_NEXT},
      {"bravePlaylistA11YPrevious", IDS_PLAYLIST_A11Y_PREVIOUS},
      {"bravePlaylistA11YShuffle", IDS_PLAYLIST_A11Y_SHUFFLE},
      {"bravePlaylistA11YToggleMuted", IDS_PLAYLIST_A11Y_TOGGLE_MUTED},
      {"bravePlaylistA11YRewind", IDS_PLAYLIST_A11Y_REWIND},
      {"bravePlaylistA11YForward", IDS_PLAYLIST_A11Y_FORWARD},
      {"bravePlaylistA11YClose", IDS_PLAYLIST_A11Y_CLOSE},
      {"bravePlaylistA11YLoopOff", IDS_PLAYLIST_A11Y_LOOP_OFF},
      {"bravePlaylistA11YLoopOne", IDS_PLAYLIST_A11Y_LOOP_ONE},
      {"bravePlaylistA11YLoopAll", IDS_PLAYLIST_A11Y_LOOP_ALL},
      {"bravePlaylistFailedToPlayTitle", IDS_PLAYLIST_FAILED_TO_PLAY_TITLE},
      {"bravePlaylistFailedToPlayDescription",
       IDS_PLAYLIST_FAILED_TO_PLAY_DESCRIPTION},
      {"bravePlaylistFailedToPlayRecover", IDS_PLAYLIST_FAILED_TO_PLAY_RECOVER},
      {"bravePlaylistAddMediaFromPage", IDS_PLAYLIST_ADD_MEDIA_FROM_PAGE},
      {"bravePlaylistAlertDismiss", IDS_PLAYLIST_ALERT_DISMISS},
  };

  for (const auto& [name, id] : kLocalizedStrings) {
    source->AddString(name, l10n_util::GetStringUTF16(id));
  }
}

}  // namespace

UntrustedPlayerUI::UntrustedPlayerUI(content::WebUI* web_ui)
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
                  "https: http://localhost;"));
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

  AddLocalizedStrings(source);
}

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

  return !PlaylistServiceFactory::GetForBrowserContext(browser_context) ||
         !user_prefs::UserPrefs::Get(browser_context)
              ->GetBoolean(playlist::kPlaylistEnabledPref);
}

PlaylistUI::PlaylistUI(content::WebUI* web_ui)
    : UntrustedWebUIController(web_ui) {
  // From MojoWebUIController
  web_ui->SetBindings(content::BINDINGS_POLICY_MOJO_WEB_UI);

  auto* source =
      CreateAndAddWebUIDataSource(web_ui, kPlaylistURL, kPlaylistGenerated,
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
    mojo::PendingRemote<playlist::mojom::PlaylistPage> page,
    mojo::PendingRemote<playlist::mojom::PlaylistServiceObserver>
        service_observer,
    mojo::PendingReceiver<playlist::mojom::PlaylistService> pending_service,
    mojo::PendingReceiver<playlist::mojom::PlaylistPageHandler> native_ui) {
  DCHECK(service_observer.is_valid());

  page_.Bind(std::move(page));

  auto* service = playlist::PlaylistServiceFactory::GetForBrowserContext(
      Profile::FromWebUI(web_ui()));
  page_handler_receivers_.Add(this, std::move(native_ui));
  service_receivers_.Add(service, std::move(pending_service));
  service->AddObserver(std::move(service_observer));

  // When WebUI calls this, mark that the page can be shown on sidebar.
  if (embedder_) {
    embedder_->ShowUI();
  }

  active_tab_tracker_ = std::make_unique<PlaylistActiveTabTracker>(
      web_ui()->GetWebContents(),
      base::BindRepeating(&PlaylistUI::OnActiveTabStateChanged,
                          weak_ptr_factory_.GetWeakPtr()));
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

void PlaylistUI::OpenSettingsPage() {
  playlist::ShowPlaylistSettings(web_ui()->GetWebContents());
}

void PlaylistUI::ShowAddMediaToPlaylistUI() {
  playlist::ShowPlaylistAddBubble(web_ui()->GetWebContents());
}

void PlaylistUI::ClosePanel() {
  playlist::ClosePanel(web_ui()->GetWebContents());
}

void PlaylistUI::ShouldShowAddMediaFromPageUI(
    ShouldShowAddMediaFromPageUICallback callback) {
  CHECK(active_tab_tracker_);
  std::move(callback).Run(active_tab_tracker_->ShouldShowAddMediaFromPageUI());
}

void PlaylistUI::OnActiveTabStateChanged(
    bool should_show_add_media_from_page_ui) {
  page_->OnActiveTabChanged(should_show_add_media_from_page_ui);
}

WEB_UI_CONTROLLER_TYPE_IMPL(PlaylistUI)

////////////////////////////////////////////////////////////////////////////////
// UntrustedPlaylistUIConfig
//
bool UntrustedPlaylistUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return base::FeatureList::IsEnabled(playlist::features::kPlaylist);
}

UntrustedPlaylistUIConfig::UntrustedPlaylistUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIUntrustedScheme,
                                  kPlaylistHost) {}

////////////////////////////////////////////////////////////////////////////////
// UntrustedPlaylistPlayerUIConfig
//
UntrustedPlaylistPlayerUIConfig::UntrustedPlaylistPlayerUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIUntrustedScheme,
                                  kPlaylistPlayerHost) {}

}  // namespace playlist
