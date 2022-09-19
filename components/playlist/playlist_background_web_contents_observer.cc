/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_background_web_contents_observer.h"

#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace playlist {

PlaylistBackgroundWebContentsObserver::PlaylistBackgroundWebContentsObserver(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<PlaylistBackgroundWebContentsObserver>(
          *web_contents) {}

PlaylistBackgroundWebContentsObserver::
    ~PlaylistBackgroundWebContentsObserver() = default;

void PlaylistBackgroundWebContentsObserver::RenderFrameCreated(
    content::RenderFrameHost* rfh) {
  if (rfh)
    GetBraveShieldsRemote(rfh)->AllowCosmeticFiltering();
}

void PlaylistBackgroundWebContentsObserver::RenderFrameDeleted(
    content::RenderFrameHost* rfh) {
  brave_shields_remotes_.erase(rfh);
}

void PlaylistBackgroundWebContentsObserver::RenderFrameHostChanged(
    content::RenderFrameHost* old_rfh,
    content::RenderFrameHost* new_rfh) {
  if (old_rfh)
    RenderFrameDeleted(old_rfh);

  if (new_rfh)
    RenderFrameCreated(new_rfh);
}

mojo::AssociatedRemote<brave_shields::mojom::BraveShields>&
PlaylistBackgroundWebContentsObserver::GetBraveShieldsRemote(
    content::RenderFrameHost* rfh) {
  if (!brave_shields_remotes_.contains(rfh)) {
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(
        &brave_shields_remotes_[rfh]);
  }

  DCHECK(brave_shields_remotes_[rfh].is_bound());
  return brave_shields_remotes_[rfh];
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistBackgroundWebContentsObserver);

}  // namespace playlist
