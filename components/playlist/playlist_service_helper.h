/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_HELPER_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_HELPER_H_

#include <vector>

#include "base/values.h"

namespace content {
class WebContents;
}  // namespace content

namespace playlist {

struct PlaylistItemInfo;

base::Value::Dict GetValueFromPlaylistItemInfo(const PlaylistItemInfo& info);
void MarkAsBackgroundWebContents(content::WebContents* web_contents);
bool IsBackgroundWebContents(content::WebContents* web_contents);

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_HELPER_H_
