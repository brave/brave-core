/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/playlist_page_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/playlist_constants.h"
#include "chrome/browser/profiles/profile.h"

#if !defined(OS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#endif

using PlaylistId = playlist::PlaylistService::PlaylistId;
using PlaylistItemId = playlist::PlaylistService::PlaylistItemId;

playlist::PlaylistService* GetPlaylistService(Profile* profile) {
  return playlist::PlaylistServiceFactory::GetForBrowserContext(profile);
}

#if !BUILDFLAG(IS_ANDROID)
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
#else
PlaylistPageHandler::PlaylistPageHandler(Profile* profile) : profile_(profile) {
  DCHECK(profile_);
  // TODO DEEP : check if we need observer for android
  // observation_.Observe(GetPlaylistService(profile_));
}
#endif

PlaylistPageHandler::~PlaylistPageHandler() = default;

mojo::PendingRemote<playlist::mojom::PageHandler>
PlaylistPageHandler::MakeRemote() {
  mojo::PendingRemote<playlist::mojom::PageHandler> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void PlaylistPageHandler::Bind(
    mojo::PendingReceiver<playlist::mojom::PageHandler> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void PlaylistPageHandler::GetAllPlaylists(
    PlaylistPageHandler::GetAllPlaylistsCallback callback) {
  std::vector<mojo::StructPtr<playlist::mojom::Playlist>> playlists;
  for (const auto& playlist : GetPlaylistService(profile_)->GetAllPlaylists()) {
    std::vector<mojo::StructPtr<playlist::mojom::PlaylistItem>> items;
    for (const auto& item : playlist.items) {
      items.push_back(playlist::mojom::PlaylistItem::New(
          item.id, item.title, GURL(item.page_src), GURL(item.media_file_path),
          GURL(item.thumbnail_path), item.media_file_cached));
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
    items.push_back(playlist::mojom::PlaylistItem::New(
        item.id, item.title, GURL(item.page_src),
        GURL("file://" + item.media_file_path),
        GURL("file://" + item.thumbnail_path), item.media_file_cached));
    LOG(ERROR) << "BravePlaylist"
               << "GetPlaylist : " << item.media_file_path << " : "
               << item.thumbnail_path;
  }
  std::move(callback).Run(playlist::mojom::Playlist::New(
      playlist->id, playlist->name, std::move(items)));
}

void PlaylistPageHandler::AddMediaFilesFromPageToPlaylist(const std::string& id,
                                                          const GURL& url) {
  GetPlaylistService(profile_)->RequestDownloadMediaFilesFromPage(id,
                                                                  url.spec());
}

void PlaylistPageHandler::AddMediaFilesFromOpenTabsToPlaylist(
    const std::string& playlist_id) {
#if defined(OS_ANDROID)
  NOTIMPLEMENTED();
#else
  auto* browser = chrome::FindLastActive();
  if (!browser) {
    LOG(ERROR) << "No active browser";
    return;
  }

  auto* tab_strip_model = browser->tab_strip_model();
  for (auto i = 0; i < tab_strip_model->count(); i++) {
    if (auto* contents = tab_strip_model->GetWebContentsAt(i);
        contents != web_contents_) {
      GetPlaylistService(profile_)->RequestDownloadMediaFilesFromContents(
          playlist_id, contents);
    }
  }
#endif  // defined(OS_ANDROID)
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
#if !BUILDFLAG(IS_ANDROID)
  page_->OnEvent(playlist::mojom::PlaylistEvent::kUpdated);
#endif
}
