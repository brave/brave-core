/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/platform/web_content_settings_client.h"

#define BRAVE_AUTOPLAY_POLICY_DOCUMENT_SHOULD_AUTOPLAY_MUTED_VIDEOS \
  if (GetAutoplayPolicyForDocument(document) ==                     \
      AutoplayPolicy::Type::kUserGestureRequired)                   \
    return false;

#define BRAVE_AUTOPLAY_POLICY_IS_GESTURE_NEEDED_FOR_PLAYBACK \
  if (IsAutoplayAllowedPerSettings())                        \
    return false;

#define BRAVE_AUTOPLAY_POLICY_IS_AUTOPLAY_ALLOWED_PER_SETTINGS          \
  bool AutoplayPolicy::IsAutoplayAllowedPerSettings() const {           \
    LocalFrame* frame = element_->GetDocument().GetFrame();             \
    if (!frame)                                                         \
      return false;                                                     \
    if (auto* settings_client = frame->GetContentSettingsClient())      \
      return settings_client->AllowAutoplay(false /* default_value */); \
    return true;                                                        \
  }

#include "../../../../../third_party/blink/renderer/core/html/media/autoplay_policy.cc"  // NOLINT

#undef BRAVE_AUTOPLAY_POLICY_DOCUMENT_SHOULD_AUTOPLAY_MUTED_VIDEOS
#undef BRAVE_AUTOPLAY_POLICY_IS_GESTURE_NEEDED_FOR_PLAYBACK
#undef BRAVE_AUTOPLAY_POLICY_IS_AUTOPLAY_ALLOWED_PER_SETTINGS
