/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/content/browser/web_contents/web_contents_impl.cc"

namespace content {

base::flat_map<MediaPlayerId, std::tuple<GURL, bool, double>>
WebContentsImpl::GetMediaMetadataByMediaPlayerIds() const {
  return media_web_contents_observer()->GetMediaMetadataByMediaPlayerIds();
}

}  // namespace content
