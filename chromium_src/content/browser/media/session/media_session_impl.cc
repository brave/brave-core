/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/content/browser/media/session/media_session_impl.cc"

namespace content {

std::optional<media_session::MediaPosition>
MediaSessionImpl::GetMediaPositionFromNormalPlayerIfPossible() {
  if (normal_players_.size() == 1 && one_shot_players_.empty() &&
      pepper_players_.empty()) {
    auto& first = normal_players_.begin()->first;
    return first.observer->GetPosition(first.player_id);
  }

  return std::nullopt;
}

}  // namespace content
