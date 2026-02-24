/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/live_caption/caption_util.h"

#define IsLiveCaptionFeatureSupported IsLiveCaptionFeatureSupported_ChromiumImpl

#include <components/live_caption/caption_util.cc>

#undef IsLiveCaptionFeatureSupported

namespace captions {

bool IsLiveCaptionFeatureSupported() {
  // Delegate to Chromium's implementation so that SODA (Speech On-Device API)
  // is available for on-device speech recognition via the WebSpeech API.
  // Live Caption UI is removed in Brave's settings (a11y_page.ts).
  return IsLiveCaptionFeatureSupported_ChromiumImpl();
}

}  // namespace captions
