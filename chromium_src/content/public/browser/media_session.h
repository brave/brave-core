/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_MEDIA_SESSION_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_MEDIA_SESSION_H_

#include <optional>

#include "content/public/browser/media_player_id.h"

#define SetAudioFocusGroupId(param) \
  SetAudioFocusGroupId(param) = 0;  \
  virtual std::optional<MediaPlayerId> GetActiveMediaPlayerId() const

#include "src/content/public/browser/media_session.h"  // IWYU pragma: export

#undef SetAudioFocusGroupId

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_MEDIA_SESSION_H_
