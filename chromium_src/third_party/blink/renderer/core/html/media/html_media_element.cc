/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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
#include "src/third_party/blink/renderer/core/html/media/html_media_element.cc"
#undef RequestAutoplayUnmute
