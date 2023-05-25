/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/playlist_tab_helper.h"

#include <utility>

#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/common/features.h"

namespace playlist {

// static
void PlaylistTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
  if (!base::FeatureList::IsEnabled(playlist::features::kPlaylist)) {
    return;
  }

  // |service| could be null when the service is not supported for the
  // browser context.
  if (auto* service = PlaylistServiceFactory::GetForBrowserContext(
          contents->GetBrowserContext())) {
    content::WebContentsUserData<PlaylistTabHelper>::CreateForWebContents(
        contents, service);
  }
}

PlaylistTabHelper::PlaylistTabHelper(content::WebContents* contents,
                                     PlaylistService* service)
    : WebContentsUserData(*contents), service_(service) {
  Observe(contents);
  CHECK(service_);
  service_->AddObserver(
      playlist_observer_receiver_.InitWithNewPipeAndPassRemote());
}

PlaylistTabHelper::~PlaylistTabHelper() = default;

void PlaylistTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // We're resetting data on finish, not on start, because navigation could fail
  // or aborted.
  ResetData();
}

void PlaylistTabHelper::DOMContentLoaded(
    content::RenderFrameHost* render_frame_host) {
  FindMediaFromCurrentContents();
}

void PlaylistTabHelper::OnMediaFilesUpdated(
    const GURL& url,
    std::vector<mojom::PlaylistItemPtr> items) {
  OnFoundMediaFromContents(url, std::move(items));
}

void PlaylistTabHelper::ResetData() {
  found_items_.clear();
}

void PlaylistTabHelper::FindMediaFromCurrentContents() {
  CHECK(service_);

  service_->FindMediaFilesFromContents(
      web_contents(),
      base::BindOnce(&PlaylistTabHelper::OnFoundMediaFromContents,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PlaylistTabHelper::OnFoundMediaFromContents(
    const GURL& url,
    std::vector<mojom::PlaylistItemPtr> items) {
  if (url != web_contents()->GetVisibleURL()) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << " item count : " << items.size();

  found_items_.insert(found_items_.end(),
                      std::make_move_iterator(items.begin()),
                      std::make_move_iterator(items.end()));
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistTabHelper);

}  // namespace playlist
