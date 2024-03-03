/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_background_webcontents_helper.h"

#include <set>

#include "base/feature_list.h"
#include "base/logging.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "net/base/schemeful_site.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "url/gurl.h"

namespace playlist {

PlaylistBackgroundWebContentsHelper::~PlaylistBackgroundWebContentsHelper() =
    default;

// static
bool PlaylistBackgroundWebContentsHelper::ShouldUseFakeUA(const GURL& url) {
  if (base::FeatureList::IsEnabled(features::kPlaylistFakeUA)) {
    return true;
  }

  static const std::set kSites{
      net::SchemefulSite(GURL("https://ted.com")),
      net::SchemefulSite(GURL("https://marthastewart.com")),
      net::SchemefulSite(GURL("https://bbcgoodfood.com")),
      net::SchemefulSite(GURL("https://rumble.com/")),
      net::SchemefulSite(GURL(
          "https://brighteon.com"))  // We only support audio for this site.
  };

  return kSites.contains(net::SchemefulSite(url));
}

bool PlaylistBackgroundWebContentsHelper::ShouldSuppressMediaSourceAPI(
    const GURL& url) {
  static const std::set kSites{
      net::SchemefulSite(GURL("https://youtube.com")),
      net::SchemefulSite(GURL("https://vimeo.com")),
      net::SchemefulSite(GURL("https://ted.com")),
      net::SchemefulSite(GURL("https://bitchute.com")),
      net::SchemefulSite(GURL("https://marthastewart.com")),
      net::SchemefulSite(GURL("https://bbcgoodfood.com")),
      net::SchemefulSite(GURL("https://rumble.com/")),
      net::SchemefulSite(GURL("https://brighteon.com"))};

  return kSites.contains(net::SchemefulSite(url));
}

PlaylistBackgroundWebContentsHelper::PlaylistBackgroundWebContentsHelper(
    content::WebContents* web_contents,
    PlaylistService* service)
    : content::WebContentsUserData<PlaylistBackgroundWebContentsHelper>(
          *web_contents),
      content::WebContentsObserver(web_contents),
      service_(service) {}

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
  frame_observer_config->AddMediaDetector(
      service_->GetMediaDetectorScript(url));

  if (ShouldSuppressMediaSourceAPI(url)) {
    frame_observer_config->AddMediaSourceAPISuppressor(
        service_->GetMediaSourceAPISuppressorScript());
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistBackgroundWebContentsHelper);

}  // namespace playlist
