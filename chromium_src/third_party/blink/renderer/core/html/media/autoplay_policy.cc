/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"

namespace {

bool IsAutoplayAllowedForElement(
    blink::Member<blink::HTMLMediaElement> element) {
  blink::LocalFrame* frame = element->GetDocument().GetFrame();
  if (!frame)
    return false;
  if (auto* settings_client = frame->GetContentSettingsClient())
    return settings_client->AllowAutoplay(true /* default_value */);
  return true;
}

}  // namespace

#define BRAVE_AUTOPLAY_POLICY_IS_GESTURE_NEEDED_FOR_PLAYBACK \
  if (!IsAutoplayAllowedForElement(element_))                \
    return true;

#include "../../../../../../../../third_party/blink/renderer/core/html/media/autoplay_policy.cc"

#undef BRAVE_AUTOPLAY_POLICY_IS_GESTURE_NEEDED_FOR_PLAYBACK
