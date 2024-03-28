/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_background_web_contents_helper.h"

#include <utility>

#include "base/logging.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "url/gurl.h"

namespace playlist {

// static
void PlaylistBackgroundWebContentsHelper::CreateForWebContents(
    content::WebContents* web_contents,
    PlaylistService* service,
    PlaylistMediaHandler::OnceCallback on_media_detected_callback) {
  content::WebContentsUserData<
      PlaylistBackgroundWebContentsHelper>::CreateForWebContents(web_contents,
                                                                 service);
  PlaylistMediaHandler::CreateForWebContents(
      web_contents, std::move(on_media_detected_callback));
}

PlaylistBackgroundWebContentsHelper::~PlaylistBackgroundWebContentsHelper() =
    default;

PlaylistBackgroundWebContentsHelper::PlaylistBackgroundWebContentsHelper(
    content::WebContents* web_contents,
    PlaylistService* service)
    : content::WebContentsUserData<PlaylistBackgroundWebContentsHelper>(
          *web_contents),
      content::WebContentsObserver(web_contents),
      service_(service) {
  CHECK(service_);
}

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
  frame_observer_config->AddMediaSourceAPISuppressor(
      service_->GetMediaSourceAPISuppressorScript());
  frame_observer_config->AddMediaDetector(
      service_->GetMediaDetectorScript(url));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistBackgroundWebContentsHelper);

}  // namespace playlist
