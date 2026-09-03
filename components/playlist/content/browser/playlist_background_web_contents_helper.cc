/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_background_web_contents_helper.h"

#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/playlist/content/browser/playlist_constants.h"
#include "brave/components/playlist/content/browser/playlist_network_media_detector.h"
#include "brave/components/playlist/content/browser/playlist_service.h"
#include "brave/components/playlist/core/common/features.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
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
  content::WebContentsUserData<PlaylistBackgroundWebContentsHelper>::
      CreateForWebContents(web_contents, service,
                           std::move(on_media_detected_callback));

  auto* helper =
      PlaylistBackgroundWebContentsHelper::FromWebContents(web_contents);
  CHECK(helper);
  PlaylistMediaHandler::CreateForWebContents(
      web_contents,
      base::BindOnce(&PlaylistBackgroundWebContentsHelper::OnMediaDetected,
                     helper->weak_factory_.GetWeakPtr()));

  if (base::FeatureList::IsEnabled(features::kPlaylistServiceV2)) {
    PlaylistNetworkMediaDetector::CreateForWebContents(
        web_contents, base::BindRepeating(
                          &PlaylistBackgroundWebContentsHelper::OnMediaDetected,
                          helper->weak_factory_.GetWeakPtr()));
  }
}

PlaylistBackgroundWebContentsHelper::~PlaylistBackgroundWebContentsHelper() =
    default;

PlaylistBackgroundWebContentsHelper::PlaylistBackgroundWebContentsHelper(
    content::WebContents* web_contents,
    PlaylistService* service,
    PlaylistMediaHandler::OnceCallback on_media_detected_callback)
    : content::WebContentsUserData<PlaylistBackgroundWebContentsHelper>(
          *web_contents),
      content::WebContentsObserver(web_contents),
      service_(service),
      on_media_detected_callback_(std::move(on_media_detected_callback)) {
  CHECK(service_);
  CHECK(on_media_detected_callback_);
}

void PlaylistBackgroundWebContentsHelper::OnMediaDetected(
    GURL url,
    std::vector<mojom::PlaylistItemPtr> items) {
  if (!on_media_detected_callback_ || items.empty()) {
    return;
  }

  std::move(on_media_detected_callback_).Run(std::move(url), std::move(items));
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

  if (base::FeatureList::IsEnabled(features::kPlaylistServiceV2) &&
      !IsYoutubeLegacyPlaylistSite(url)) {
    frame_observer_config->ClearMediaScripts();
    return;
  }

  frame_observer_config->AddMediaSourceAPISuppressor(
      service_->GetMediaSourceAPISuppressorScript());
  frame_observer_config->AddMediaDetector(
      service_->GetMediaDetectorScript(url));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistBackgroundWebContentsHelper);

}  // namespace playlist
