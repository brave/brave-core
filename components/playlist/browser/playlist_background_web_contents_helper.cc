/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_background_web_contents_helper.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/logging.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "url/gurl.h"

namespace playlist {

PlaylistBackgroundWebContentsHelper::~PlaylistBackgroundWebContentsHelper() =
    default;

PlaylistBackgroundWebContentsHelper::PlaylistBackgroundWebContentsHelper(
    content::WebContents* web_contents,
    base::TimeDelta duration,
    base::OnceCallback<void(GURL, bool)> callback)
    : content::WebContentsUserData<PlaylistBackgroundWebContentsHelper>(
          *web_contents),
      content::WebContentsObserver(web_contents),
      duration_(std::move(duration)),
      callback_(std::move(callback)) {}

void PlaylistBackgroundWebContentsHelper::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  DVLOG(2) << __FUNCTION__;

  CHECK(navigation_handle);
  if (!navigation_handle->GetURL().SchemeIsHTTPOrHTTPS()) {
    return;
  }

  mojo::AssociatedRemote<mojom::PlaylistRenderFrameObserverConfigurator>
      frame_observer_config;
  navigation_handle->GetRenderFrameHost()
      ->GetRemoteAssociatedInterfaces()
      ->GetInterface(&frame_observer_config);
  frame_observer_config->EnableMediaSourceAPISuppressor();

  timer_.Start(FROM_HERE, base::Milliseconds(500), this,
               &PlaylistBackgroundWebContentsHelper::GetMediaMetadata);
}

void PlaylistBackgroundWebContentsHelper::GetMediaMetadata() {
  for (auto&& [media_player_id, metadata] :
       GetWebContents().GetMediaMetadataByMediaPlayerIds()) {
    auto [url, is_media_source, duration] = std::move(metadata);

    DVLOG(-1) << "Media player (" << media_player_id.frame_routing_id << ", "
              << media_player_id.delegate_id
              << ") URL: " << (!url.is_valid() ? "not set" : url.spec())
              << " (duration: " << base::Seconds(duration) << ')';

    if (!url.is_valid()) {
      continue;
    }

    if (std::abs((duration_ - base::Seconds(duration)).InSeconds()) < 5) {
      DVLOG(-1) << "URL extracted from the background: " << url;
      return std::move(callback_).Run(std::move(url), is_media_source);
    }
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistBackgroundWebContentsHelper);

}  // namespace playlist
