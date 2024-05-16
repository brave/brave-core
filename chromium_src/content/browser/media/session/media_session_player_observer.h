/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_PLAYER_OBSERVER_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_PLAYER_OBSERVER_H_

#include "content/public/browser/media_player_id.h"

#define GetMediaContentType        \
  GetMediaContentType() const = 0; \
  virtual MediaPlayerId GetMediaPlayerId

#include "src/content/browser/media/session/media_session_player_observer.h"  // IWYU pragma: export

#undef GetMediaContentType

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_PLAYER_OBSERVER_H_
