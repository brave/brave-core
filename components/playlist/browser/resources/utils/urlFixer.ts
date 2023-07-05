/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { PlaylistItem } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'
import { Url } from 'gen/url/mojom/url.mojom.m'

export function fixUpThumbnailURL (item: PlaylistItem) {
  if (!item.thumbnailPath?.url) {
    return
  }

  if (
    item.thumbnailPath.url.startsWith('http://') ||
    item.thumbnailPath.url.startsWith('https://')
  ) {
    // It's not cached yet. We should clear the source url and show default thumbnailPath.
    item.thumbnailPath = new Url()
    return
  }
  // Set a url with chrome-untrusted:// protocol corresponding to the the local file.
  item.thumbnailPath.url = `chrome-untrusted://playlist-data/${item.id}/thumbnail/`
}

export function fixUpMediaURL(item: PlaylistItem) {
  if (!item.cached) {
    // When the media is not cached, we just try playing the given url
    return
  }

  // Set a url with chrome-untrusted:// protocol corresponding to the the local file.
  item.mediaPath.url = `chrome-untrusted://playlist-data/${item.id}/media/`
}
