/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_HANDLER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/global_routing_id.h"

namespace playlist {

class PlaylistService;
class PlaylistMediaHandler: public mojom::PlaylistMediaHandler{
 public:
  PlaylistMediaHandler(
      content::GlobalRenderFrameHostId frame_id,
      const base::WeakPtr<PlaylistService>& service);
  ~PlaylistMediaHandler() override;

  // mojom::PlaylistMediaHandler:
  void OnMediaUpdatedFromRenderFrame() override;

 private:
  content::GlobalRenderFrameHostId frame_id_;
  base::WeakPtr<PlaylistService> service_;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_HANDLER_H_
