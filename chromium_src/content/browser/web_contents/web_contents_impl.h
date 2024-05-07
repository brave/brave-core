/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_H_

#include <tuple>
#include <utility>

#include "base/containers/flat_map.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace content {
class MediaWebContentsObserver;
}

#define GetBackForwardTransitionAnimationManager()              \
  GetBackForwardTransitionAnimationManager() override;          \
  base::flat_map<MediaPlayerId, std::tuple<GURL, bool, double>> \
  GetMediaMetadataByMediaPlayerIds() const

#define media_web_contents_observer()                        \
  media_web_contents_observer() {                            \
    return const_cast<MediaWebContentsObserver*>(            \
        std::as_const(*this).media_web_contents_observer()); \
  }                                                          \
  const MediaWebContentsObserver* media_web_contents_observer() const

#include "src/content/browser/web_contents/web_contents_impl.h"  // IWYU pragma: export

#undef media_web_contents_observer
#undef GetBackForwardTransitionAnimationManager

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_H_
