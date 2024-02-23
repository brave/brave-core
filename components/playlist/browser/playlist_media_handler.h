/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_RESPONDER_IMPL_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_RESPONDER_IMPL_H_

#include "base/functional/callback_forward.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/render_frame_host_receiver_set.h"

namespace playlist {

class PlaylistService;

class PlaylistMediaHandler : public mojom::PlaylistMediaResponder {
 public:
  PlaylistMediaHandler(content::WebContents* contents,
                       base::RepeatingCallback<void(base::Value, const GURL&)>
                           on_media_detected);

  ~PlaylistMediaHandler() override;

  void OnMediaDetected(base::Value media) override;

  void BindMediaResponder(
      mojo::PendingAssociatedReceiver<mojom::PlaylistMediaResponder> receiver,
      content::RenderFrameHost* render_frame_host);

 private:
  content::RenderFrameHostReceiverSet<mojom::PlaylistMediaResponder>
      media_responder_receivers_;
  base::RepeatingCallback<void(base::Value, const GURL&)> on_media_detected_;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_RESPONDER_IMPL_H_
