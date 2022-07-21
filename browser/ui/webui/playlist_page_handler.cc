/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/playlist_page_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/playlist_constants.h"
#include "chrome/browser/profiles/profile.h"

playlist::PlaylistService* GetPlaylistService(Profile* profile) {
  return playlist::PlaylistServiceFactory::GetForBrowserContext(profile);
}

PlaylistPageHandler::PlaylistPageHandler(
    Profile* profile,
    mojo::PendingReceiver<playlist::mojom::PageHandler> pending_page_handler,
    mojo::PendingRemote<playlist::mojom::Page> pending_page)
    : profile_(profile),
      page_(std::move(pending_page)),
      handler_(this, std::move(pending_page_handler)) {
  DCHECK(profile_);
  observation_.Observe(GetPlaylistService(profile_));
}

PlaylistPageHandler::~PlaylistPageHandler() = default;

void PlaylistPageHandler::GetAllPlaylists(
    PlaylistPageHandler::GetAllPlaylistsCallback callback) {
  auto value = GetPlaylistService(profile_)->GetAllPlaylistItems();
  DCHECK(value.is_list());

  std::vector<mojo::StructPtr<playlist::mojom::Playlist>> playlists;
  playlists.push_back(playlist::mojom::Playlist::New());
  auto& playlist = playlists.back();
  playlist->name = "default";
  for (const auto& item : value.GetList()) {
    auto* dict = item.GetIfDict();
    DCHECK(dict);
    DCHECK(dict->contains(playlist::kPlaylistItemIDKey));
    DCHECK(dict->contains(playlist::kPlaylistItemTitleKey));
    DCHECK(dict->contains(playlist::kPlaylistItemMediaFilePathKey));
    DCHECK(dict->contains(playlist::kPlaylistItemThumbnailPathKey));

    playlist->items.push_back(playlist::mojom::PlaylistItem::New(
        *dict->FindString(playlist::kPlaylistItemIDKey) /* = id */,
        *dict->FindString(playlist::kPlaylistItemTitleKey) /* =  name */,
        GURL(*dict->FindString(
            playlist::kPlaylistItemMediaFilePathKey)) /* = media_path */,
        GURL(*dict->FindString(
            playlist::
                kPlaylistItemThumbnailPathKey)) /* = ::GURL& thumbnail_path */,
        *dict->FindBool(playlist::kPlaylistItemReadyKey) /* = ready */));
  }

  std::move(callback).Run(std::move(playlists));
}

void PlaylistPageHandler::GetPlaylist(
    const std::string& id,
    PlaylistPageHandler::GetPlaylistCallback callback) {
  // TODO(sko) Get items for playlist with |id|.
  auto value = GetPlaylistService(profile_)->GetAllPlaylistItems();
  DCHECK(value.is_list());
  playlist::mojom::Playlist playlist;
  playlist.name = "default";
  for (const auto& item : value.GetList()) {
    auto* dict = item.GetIfDict();
    DCHECK(dict);
    DCHECK(dict->contains(playlist::kPlaylistItemIDKey));
    DCHECK(dict->contains(playlist::kPlaylistItemTitleKey));
    DCHECK(dict->contains(playlist::kPlaylistItemMediaFilePathKey));
    DCHECK(dict->contains(playlist::kPlaylistItemThumbnailPathKey));

    playlist.items.push_back(playlist::mojom::PlaylistItem::New(
        *dict->FindString(playlist::kPlaylistItemIDKey) /* = id */,
        *dict->FindString(playlist::kPlaylistItemTitleKey) /* =  name */,
        GURL(*dict->FindString(
            playlist::kPlaylistItemMediaFilePathKey)) /* = media_path */,
        GURL(*dict->FindString(
            playlist::
                kPlaylistItemThumbnailPathKey)) /* = ::GURL& thumbnail_path */,
        *dict->FindBool(playlist::kPlaylistItemReadyKey) /* = ready */));
  }

  std::move(callback).Run(playlist.Clone());
}

void PlaylistPageHandler::AddMediaFilesFromPageToPlaylist(const std::string& id,
                                                          const GURL& url) {
  GetPlaylistService(profile_)->RequestDownloadMediaFilesFromPage(id,
                                                                  url.spec());
}

void PlaylistPageHandler::RemoveItemFromPlaylist(const std::string& playlist_id,
                                                 const std::string& item_id) {
  GetPlaylistService(profile_)->RemoveItemFromPlaylist(playlist_id, item_id);
}

void PlaylistPageHandler::OnPlaylistItemStatusChanged(
    const playlist::PlaylistItemChangeParams& params) {
  page_->OnEvent(playlist::mojom::PlaylistEvent::kItemAdded);
}
