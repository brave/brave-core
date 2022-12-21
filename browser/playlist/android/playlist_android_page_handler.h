/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLIST_ANDROID_PLAYLIST_ANDROID_PAGE_HANDLER_H_
#define BRAVE_BROWSER_PLAYLIST_ANDROID_PLAYLIST_ANDROID_PAGE_HANDLER_H_

#include "brave/browser/playlist/playlist_page_handler.h"
#include "brave/components/playlist/mojom/playlist.mojom.h"
#include "brave/components/playlist/playlist_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class Profile;

class PlaylistAndroidPageHandler : public PlaylistPageHandler,
                                   public KeyedService {
 public:
  explicit PlaylistAndroidPageHandler(Profile* profile);
  mojo::PendingRemote<playlist::mojom::PageHandler> MakeRemote();
  ~PlaylistAndroidPageHandler() override;

 private:
  mojo::ReceiverSet<playlist::mojom::PageHandler> receivers_;
};

#endif  // BRAVE_BROWSER_PLAYLIST_ANDROID_PLAYLIST_ANDROID_PAGE_HANDLER_H_
