/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <third_party/blink/renderer/modules/breakout_box/media_stream_audio_track_underlying_source.cc>

namespace blink {

// The feature was disabled by a Finch kill switch in m150.
// crbug.com/485217838 might have some extra details.
OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kBreakoutBoxExposePageRelativeAudioCaptureTime,
     base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace blink
