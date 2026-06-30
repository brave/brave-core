/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace blink {
namespace {

bool TreeHasUserActivation(Frame* frame) {
  for (Frame* frame_video = frame; frame_video;
       frame_video = frame_video->Tree().Parent()) {
    if (frame_video->HasStickyUserActivation()) {
      return true;
    }

    if (auto* local = DynamicTo<LocalFrame>(frame_video)) {
      if (LocalFrame::HasTransientUserActivation(local)) {
        return true;
      }
    }
  }

  return false;
}

bool IsAutoplayAllowedForFrame(LocalFrame* frame, bool play_requested) {
  if (!frame) {
    return false;
  }

  if (TreeHasUserActivation(frame)) {
    return true;
  }

  if (auto* settings_client = frame->GetContentSettingsClient()) {
    bool allow_autoplay = settings_client->AllowAutoplay(play_requested);
    // Clear it in order to block media when refresh or navigate
    if (!allow_autoplay) {
      frame->ClearUserActivation();
    }
    return allow_autoplay;
  }
  return true;
}

bool IsAutoplayAllowedForDocument(const Document& document) {
  return IsAutoplayAllowedForFrame(document.GetFrame(), false);
}

bool IsAutoplayAllowedForElement(Member<HTMLMediaElement> element) {
  return IsAutoplayAllowedForFrame(element->GetDocument().GetFrame(), true);
}

}  // namespace
}  // namespace blink

#define BRAVE_AUTOPLAY_POLICY_IS_GESTURE_NEEDED_FOR_PLAYBACK \
  if (!IsAutoplayAllowedForElement(element_))                \
    return true;

#define BRAVE_GET_AUTOPLAY_POLICY_FOR_DOCUMENT \
  if (!IsAutoplayAllowedForDocument(document)) \
    return Type::kUserGestureRequired;

#include <third_party/blink/renderer/core/html/media/autoplay_policy.cc>

#undef BRAVE_AUTOPLAY_POLICY_IS_GESTURE_NEEDED_FOR_PLAYBACK
#undef BRAVE_GET_AUTOPLAY_POLICY_FOR_DOCUMENT
