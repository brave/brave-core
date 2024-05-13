/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_background_web_contents_helper.h"

#include <utility>

#include "base/logging.h"
#include "base/time/time.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "services/media_session/public/mojom/media_session.mojom.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "url/gurl.h"

namespace playlist {

PlaylistBackgroundWebContentsHelper::~PlaylistBackgroundWebContentsHelper() =
    default;

PlaylistBackgroundWebContentsHelper::PlaylistBackgroundWebContentsHelper(
    content::WebContents* web_contents,
    base::OnceCallback<void(GURL, bool)> callback)
    : content::WebContentsUserData<PlaylistBackgroundWebContentsHelper>(
          *web_contents),
      content::WebContentsObserver(web_contents),
      callback_(std::move(callback)) {}

void PlaylistBackgroundWebContentsHelper::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  DVLOG(2) << __FUNCTION__;

  DCHECK(navigation_handle);
  if (!navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }

  const GURL url = navigation_handle->GetURL();
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  mojo::AssociatedRemote<mojom::PlaylistRenderFrameObserverConfigurator>
      frame_observer_config;
  navigation_handle->GetRenderFrameHost()
      ->GetRemoteAssociatedInterfaces()
      ->GetInterface(&frame_observer_config);
  frame_observer_config->EnableMediaSourceAPISuppressor();
}

void PlaylistBackgroundWebContentsHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // TODO: use heuristics on when the site finished loading the page
  timer_.Start(FROM_HERE, base::Seconds(2), this,
               &PlaylistBackgroundWebContentsHelper::GetLoadedUrl);
}

void PlaylistBackgroundWebContentsHelper::GetLoadedUrl() {
  auto urls = web_contents()->GetLoadedUrlByMediaPlayer();
  if (urls.empty()) {
    return std::move(callback_).Run({}, {});
  }

  const auto url_it = std::max_element(
      urls.begin(), urls.end(), [](const auto& e1, const auto& e2) {
        return e1.second.first.spec().size() < e2.second.first.spec().size();
      });

  // TODO(sszaloki): do url.is_valid() check here!!!
  DVLOG(-1) << "URL extracted from the background: " << url_it->second.first;

  std::move(callback_).Run(url_it->second.first, url_it->second.second);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistBackgroundWebContentsHelper);

}  // namespace playlist
