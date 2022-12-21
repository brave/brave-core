/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/playlist_page_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/json/values_util.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_service_helper.h"
#include "brave/components/playlist/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/prefs/pref_service.h"

using PlaylistId = playlist::PlaylistService::PlaylistId;
using PlaylistItemId = playlist::PlaylistService::PlaylistItemId;

playlist::PlaylistService* GetPlaylistService(Profile* profile) {
  return playlist::PlaylistServiceFactory::GetForBrowserContext(profile);
}

PlaylistPageHandler::PlaylistPageHandler(
    Profile* profile,
    content::WebContents* contents,
    mojo::PendingReceiver<playlist::mojom::PageHandler> pending_page_handler,
    mojo::PendingRemote<playlist::mojom::Page> pending_page)
    : profile_(profile),
      web_contents_(contents),
      page_(std::move(pending_page)),
      handler_(this, std::move(pending_page_handler)) {
  DCHECK(profile_);
  observation_.Observe(GetPlaylistService(profile_));
}

PlaylistPageHandler::~PlaylistPageHandler() = default;

void PlaylistPageHandler::GetAllPlaylists(
    PlaylistPageHandler::GetAllPlaylistsCallback callback) {
  std::vector<mojo::StructPtr<playlist::mojom::Playlist>> playlists;
  for (const auto& playlist : GetPlaylistService(profile_)->GetAllPlaylists()) {
    std::vector<mojo::StructPtr<playlist::mojom::PlaylistItem>> items;
    for (const auto& item : playlist.items) {
      items.push_back(playlist::GetPlaylistItemMojoFromInfo(item));
    }
    playlists.push_back(playlist::mojom::Playlist::New(
        playlist.id, playlist.name, std::move(items)));
  }

  std::move(callback).Run(std::move(playlists));
}

void PlaylistPageHandler::GetPlaylist(
    const std::string& id,
    PlaylistPageHandler::GetPlaylistCallback callback) {
  const auto& playlist = GetPlaylistService(profile_)->GetPlaylist(id);
  if (!playlist.has_value()) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::vector<mojo::StructPtr<playlist::mojom::PlaylistItem>> items;
  for (const auto& item : playlist->items) {
    items.push_back(playlist::GetPlaylistItemMojoFromInfo(item));
  }
  std::move(callback).Run(playlist::mojom::Playlist::New(
      playlist->id, playlist->name, std::move(items)));
}

void PlaylistPageHandler::AddMediaFilesFromPageToPlaylist(const std::string& id,
                                                          const GURL& url) {
  GetPlaylistService(profile_)->AddMediaFilesFromPageToPlaylist(
      id, url.spec(),
      profile_->GetPrefs()->GetBoolean(playlist::kPlaylistCacheByDefault));
}

void PlaylistPageHandler::AddMediaFilesFromOpenTabsToPlaylist(
    const std::string& playlist_id) {
  auto* browser = chrome::FindLastActive();
  if (!browser) {
    LOG(ERROR) << "No active browser";
    return;
  }

  auto* tab_strip_model = browser->tab_strip_model();
  for (auto i = 0; i < tab_strip_model->count(); i++) {
    if (auto* contents = tab_strip_model->GetWebContentsAt(i);
        contents != web_contents_) {
      GetPlaylistService(profile_)->AddMediaFilesFromContentsToPlaylist(
          playlist_id, contents,
          profile_->GetPrefs()->GetBoolean(playlist::kPlaylistCacheByDefault));
    }
  }
}

void PlaylistPageHandler::RemoveItemFromPlaylist(const std::string& playlist_id,
                                                 const std::string& item_id) {
  GetPlaylistService(profile_)->RemoveItemFromPlaylist(PlaylistId(playlist_id),
                                                       PlaylistItemId(item_id));
}

void PlaylistPageHandler::MoveItem(const std::string& from_playlist_id,
                                   const std::string& to_playlist_id,
                                   const std::string& item_id) {
  GetPlaylistService(profile_)->MoveItem(PlaylistId(from_playlist_id),
                                         PlaylistId(to_playlist_id),
                                         PlaylistItemId(item_id));
}

void PlaylistPageHandler::UpdateItem(playlist::mojom::PlaylistItemPtr item) {
  GetPlaylistService(profile_)->UpdateItem(
      playlist::GetPlaylistItemInfoFromMojo(item));
}

void PlaylistPageHandler::RecoverLocalDataForItem(const std::string& item_id) {
  GetPlaylistService(profile_)->RecoverPlaylistItem(item_id);
}

void PlaylistPageHandler::RemoveLocalDataForItem(const std::string& item_id) {
  GetPlaylistService(profile_)->DeletePlaylistLocalData(item_id);
}

void PlaylistPageHandler::CreatePlaylist(
    playlist::mojom::PlaylistPtr playlist) {
  playlist::PlaylistInfo info;
  info.name = playlist->name;
  GetPlaylistService(profile_)->CreatePlaylist(info);
}

void PlaylistPageHandler::RemovePlaylist(const std::string& playlist_id) {
  GetPlaylistService(profile_)->RemovePlaylist(playlist_id);
}

void PlaylistPageHandler::OnPlaylistStatusChanged(
    const playlist::PlaylistChangeParams& params) {
  // TODO(sko) Send proper events based on |params|
  page_->OnEvent(playlist::mojom::PlaylistEvent::kUpdated);
}

void PlaylistPageHandler::OnMediaFileDownloadProgressed(
    const std::string& id,
    int64_t total_bytes,
    int64_t received_bytes,
    int percent_complete,
    base::TimeDelta time_remaining) {
  page_->OnMediaFileDownloadProgressed(
      id, total_bytes, received_bytes, percent_complete,
      base::TimeDeltaToValue(time_remaining).GetString());
}
