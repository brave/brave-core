/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_THUMBNAIL_PROVIDER_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_THUMBNAIL_PROVIDER_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "ui/gfx/image/image.h"

namespace playlist {
class PlaylistService;
class PlaylistTabHelper;
}  // namespace playlist

class ThumbnailProvider final {
 public:
  explicit ThumbnailProvider(playlist::PlaylistService& service);
  explicit ThumbnailProvider(playlist::PlaylistTabHelper* tab_helper);

  ThumbnailProvider(ThumbnailProvider&) = delete;
  ThumbnailProvider& operator=(ThumbnailProvider&) = delete;
  ~ThumbnailProvider();

  void GetThumbnail(const playlist::mojom::PlaylistItemPtr& item,
                    base::OnceCallback<void(const gfx::Image&)> callback);
  void GetThumbnail(const playlist::mojom::PlaylistPtr& list,
                    base::OnceCallback<void(const gfx::Image&)> callback);

 private:
  void OnGotThumbnail(const std::string& id,
                      bool from_network,
                      base::OnceCallback<void(const gfx::Image&)> callback,
                      gfx::Image thumbnail);

  raw_ref<playlist::PlaylistService> service_;

  base::WeakPtrFactory<ThumbnailProvider> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_THUMBNAIL_PROVIDER_H_
