/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "media/media_buildflags.h"

#define IsDecoderBuiltInVideoCodec IsDecoderBuiltInVideoCodec_ChromiumImpl
#define IsDefaultDecoderSupportedVideoType \
  IsDefaultDecoderSupportedVideoType_ChromiumImpl

#include <media/base/supported_types.cc>

#undef IsDefaultDecoderSupportedVideoType
#undef IsDecoderBuiltInVideoCodec

namespace media {

MEDIA_EXPORT bool IsDecoderBuiltInVideoCodec(VideoCodec codec) {
#if BUILDFLAG(IS_LINUX) && BUILDFLAG(ENABLE_PLATFORM_HEVC)
  if (codec == VideoCodec::kHEVC) {
    return true;
  }
#endif
  return IsDecoderBuiltInVideoCodec_ChromiumImpl(codec);
}

// On Linux, upstream gates HEVC support on a hardware decoder registering its
// profiles via the supplemental profile cache (the PLATFORM_HAS_OPTIONAL_HEVC_
// DECODE_SUPPORT path in IsDecoderHevcProfileSupported). We ship a bundled
// FFmpeg HEVC software decoder, so advertise HEVC as supported when the
// upstream check rejects it for that reason.
MEDIA_EXPORT bool IsDefaultDecoderSupportedVideoType(const VideoType& type) {
  const bool upstream_result =
      IsDefaultDecoderSupportedVideoType_ChromiumImpl(type);
#if BUILDFLAG(IS_LINUX) && BUILDFLAG(ENABLE_PLATFORM_HEVC)
  if (!upstream_result && type.codec == VideoCodec::kHEVC) {
    return true;
  }
#endif
  return upstream_result;
}

}  // namespace media
