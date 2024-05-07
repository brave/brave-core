/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_MEDIA_MEDIA_WEB_CONTENTS_OBSERVER_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_MEDIA_MEDIA_WEB_CONTENTS_OBSERVER_H_

#include <tuple>

#include "base/containers/flat_map.h"
#include "content/browser/media/session/media_session_controllers_manager.h"
#include "content/public/browser/media_player_id.h"
#include "media/mojo/mojom/media_player.mojom.h"
#include "url/gurl.h"

#define SuspendAllMediaPlayers()                                \
  SuspendAllMediaPlayers();                                     \
  base::flat_map<MediaPlayerId, std::tuple<GURL, bool, double>> \
  GetMediaMetadataByMediaPlayerIds() const

#define OnVideoVisibilityChanged(param)     \
  OnVideoVisibilityChanged(param) override; \
                                            \
 private:                                   \
  GURL url_;                                \
  bool is_media_source_ = false;            \
  double duration_ = 0.0;                   \
                                            \
 public:                                    \
  const GURL& GetUrl() const;               \
  bool GetIsMediaSource() const;            \
  double GetDuration() const;               \
  void OnMediaLoaded(const GURL& url, bool is_media_source, double duration)

#include "src/content/browser/media/media_web_contents_observer.h"  // IWYU pragma: export

#undef OnVideoVisibilityChanged
#undef SuspendAllMediaPlayers

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_MEDIA_MEDIA_WEB_CONTENTS_OBSERVER_H_
