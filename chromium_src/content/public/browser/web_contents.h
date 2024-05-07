/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_H_

#include <tuple>

#include "base/containers/flat_map.h"
#include "content/public/browser/media_player_id.h"
#include "url/gurl.h"

#define GetBackForwardTransitionAnimationManager()                      \
  GetBackForwardTransitionAnimationManager() = 0;                       \
  virtual base::flat_map<MediaPlayerId, std::tuple<GURL, bool, double>> \
  GetMediaMetadataByMediaPlayerIds() const

#include "src/content/public/browser/web_contents.h"  // IWYU pragma: export

#undef GetBackForwardTransitionAnimationManager

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_H_
