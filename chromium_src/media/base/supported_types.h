/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_MEDIA_BASE_SUPPORTED_TYPES_H_
#define BRAVE_CHROMIUM_SRC_MEDIA_BASE_SUPPORTED_TYPES_H_

#include <media/base/supported_types.h>  // IWYU pragma: export

namespace media {

// True when this build includes a software HEVC decoder via FFmpeg. Brave
// enables HEVC on Linux to cover NVIDIA GPUs where Chromium's VA-API hardware
// decode path is unusable.
MEDIA_EXPORT bool FFmpegSupportsHEVC();

// Convenience wrapper used by switch statements that share their threading /
// codec policy between H.264 and the bundled HEVC decoder. Returns kH264 when
// `codec` is kHEVC and the FFmpeg HEVC decoder is built in, otherwise returns
// `codec` unchanged.
MEDIA_EXPORT VideoCodec HEVCAsH264IfFFmpegSupportsHEVC(VideoCodec codec);

}  // namespace media

#endif  // BRAVE_CHROMIUM_SRC_MEDIA_BASE_SUPPORTED_TYPES_H_
