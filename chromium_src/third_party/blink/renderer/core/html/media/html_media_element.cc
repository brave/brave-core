/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/html/media/html_media_element.h"

#include "third_party/blink/renderer/core/html/media/autoplay_policy.h"

// Do not pause video if it was blocked by autoplay policy on volume change.
bool SkipPauseIfAutoplayIsBlockedByPolicy(bool is_gesture_needed) {
  if (is_gesture_needed)
    return false;
  return true;
}

#define RequestAutoplayUnmute                                \
  RequestAutoplayUnmute() &&                                 \
      SkipPauseIfAutoplayIsBlockedByPolicy(                  \
          autoplay_policy_->IsGestureNeededForPlayback()) && \
      EffectiveMediaVolume

#define OnLoadFinished()                                                   \
  OnLoadFinished();                                                        \
  for (auto& observer : media_player_observer_remote_set_->Value()) {      \
    observer->OnMediaLoaded(                                               \
        current_src_.GetSource(),                                          \
        GetLoadType() == WebMediaPlayer::kLoadTypeMediaSource, duration_); \
  }

#include "src/third_party/blink/renderer/core/html/media/html_media_element.cc"

#undef OnLoadFinished
#undef RequestAutoplayUnmute
