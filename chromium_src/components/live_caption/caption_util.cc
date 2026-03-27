/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/live_caption/caption_util.h"

#include "base/feature_list.h"
#include "brave/components/local_ai/core/features.h"

#define IsLiveCaptionFeatureSupported IsLiveCaptionFeatureSupported_ChromiumImpl

#include <components/live_caption/caption_util.cc>

#undef IsLiveCaptionFeatureSupported

namespace captions {

bool IsLiveCaptionFeatureSupported() {
  if (base::FeatureList::IsEnabled(
          local_ai::features::kBraveOnDeviceSpeechRecognition)) {
    return true;
  }
  return false;
}

}  // namespace captions
