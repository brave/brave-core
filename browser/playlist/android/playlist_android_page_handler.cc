/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/android/playlist_android_page_handler.h"

#include "brave/browser/playlist/playlist_service_factory.h"
#include "chrome/browser/profiles/profile.h"

PlaylistAndroidPageHandler::PlaylistAndroidPageHandler(Profile* profile)
    : PlaylistPageHandler(profile) {
  // DCHECK(playlistPageHandler_);
  // TODO DEEP : check if we need observer for android
  // observation_.Observe(GetPlaylistService(profile_));
}

PlaylistAndroidPageHandler::~PlaylistAndroidPageHandler() = default;

mojo::PendingRemote<playlist::mojom::PageHandler>
PlaylistAndroidPageHandler::MakeRemote() {
  mojo::PendingRemote<playlist::mojom::PageHandler> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}
