/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_background_webcontents_helper.h"

#include <utility>

#include "base/logging.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace playlist {

PlaylistBackgroundWebContentsHelper::~PlaylistBackgroundWebContentsHelper() =
    default;

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
      media_source_api_suppressor_);
  frame_observer_config->AddMediaDetector(media_detector_);
}

base::OnceCallback<void(bool)>
PlaylistBackgroundWebContentsHelper::GetSuccessCallback() && {
  return base::DoNothing();
}

PlaylistBackgroundWebContentsHelper::PlaylistBackgroundWebContentsHelper(
    content::WebContents* web_contents,
    PlaylistService* service,
    const std::string& media_source_api_suppressor,
    const std::string& media_detector)
    : content::WebContentsUserData<PlaylistBackgroundWebContentsHelper>(
          *web_contents),
      content::WebContentsObserver(web_contents),
      media_source_api_suppressor_(media_source_api_suppressor),
      media_detector_(media_detector) {}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistBackgroundWebContentsHelper);

}  // namespace playlist
