/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_MEDIA_BASE_MEDIA_SWITCHES_H_
#define BRAVE_CHROMIUM_SRC_MEDIA_BASE_MEDIA_SWITCHES_H_

#include <media/base/media_switches.h>  // IWYU pragma: export

#include "media/media_buildflags.h"

namespace media {

#if BUILDFLAG(IS_LINUX) && BUILDFLAG(ENABLE_PLATFORM_HEVC)
// Killswitch for the bundled FFmpeg HEVC software decoder on Linux. Enabled by
// default; lets us disable HEVC decoding remotely (via Griffin) or locally
// (via brave://flags) without rebuilding if a regression shows up.
MEDIA_EXPORT BASE_DECLARE_FEATURE(kFFmpegSoftwareHEVCDecoder);
#endif

}  // namespace media

#endif  // BRAVE_CHROMIUM_SRC_MEDIA_BASE_MEDIA_SWITCHES_H_
