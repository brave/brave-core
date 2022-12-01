/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLIST_PLAYLIST_PAGE_HANDLER_H_
#define BRAVE_BROWSER_PLAYLIST_PLAYLIST_PAGE_HANDLER_H_

#include <string>

#include "base/scoped_observation.h"
#include "brave/components/playlist/mojom/playlist.mojom.h"
#include "brave/components/playlist/playlist_service.h"
#include "brave/components/playlist/playlist_service_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

class Profile;

class PlaylistPageHandler : public KeyedService,
                            public playlist::mojom::PageHandler,
                            public playlist::PlaylistServiceObserver {
 public:
#if !BUILDFLAG(IS_ANDROID)
  PlaylistPageHandler(
      Profile* profile,
      content::WebContents* contents,
      mojo::PendingReceiver<playlist::mojom::PageHandler> pending_page_handler,
      mojo::PendingRemote<playlist::mojom::Page> pending_page);
#else
  PlaylistPageHandler(Profile* profile);
  mojo::PendingRemote<playlist::mojom::PageHandler> MakeRemote();
  void Bind(mojo::PendingReceiver<playlist::mojom::PageHandler> receiver);
#endif
  ~PlaylistPageHandler() override;

  // playlist::mojom::PageHandler:
  void GetAllPlaylists(
      PlaylistPageHandler::GetAllPlaylistsCallback callback) override;
  void GetPlaylist(const std::string& id,
                   PlaylistPageHandler::GetPlaylistCallback callback) override;
  void AddMediaFilesFromPageToPlaylist(const std::string& playlist_id,
                                       const GURL& url) override;
  void AddMediaFilesFromOpenTabsToPlaylist(
      const std::string& playlist_id) override;
  void RemoveItemFromPlaylist(const std::string& playlist_id,
                              const std::string& item_id) override;
  void MoveItem(const std::string& from_playlist_id,
                const std::string& to_playlist_id,
                const std::string& item_id) override;
  void RecoverLocalDataForItem(const std::string& item_id) override;
  void RemoveLocalDataForItem(const std::string& item_id) override;

  void CreatePlaylist(playlist::mojom::PlaylistPtr playlist) override;
  void RemovePlaylist(const std::string& playlist_id) override;

  // playlist::PlaylistServiceObserver
  void OnPlaylistStatusChanged(
      const playlist::PlaylistChangeParams& params) override;

 private:
  raw_ptr<Profile> profile_ = nullptr;
#if !BUILDFLAG(IS_ANDROID)
  raw_ptr<content::WebContents> web_contents_ = nullptr;

  mojo::Remote<playlist::mojom::Page> page_;
  mojo::Receiver<playlist::mojom::PageHandler> handler_;
#else
  mojo::ReceiverSet<playlist::mojom::PageHandler> receivers_;
#endif

  base::ScopedObservation<playlist::PlaylistService,
                          playlist::PlaylistServiceObserver>
      observation_{this};
};

#endif  // BRAVE_BROWSER_PLAYLIST_PLAYLIST_PAGE_HANDLER_H_
