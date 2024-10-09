/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/live_caption/caption_util.h"

#define IsLiveCaptionFeatureSupported IsLiveCaptionFeatureSupported_ChromiumImpl

#include "src/components/live_caption/caption_util.cc"

#undef IsLiveCaptionFeatureSupported

namespace captions {

bool IsLiveCaptionFeatureSupported() {
  return false;
}

}  // namespace captions
