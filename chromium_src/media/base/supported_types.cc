/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "media/base/supported_types.h"

#include "base/feature_list.h"
#include "build/build_config.h"
#include "media/base/media_switches.h"
#include "media/media_buildflags.h"

namespace media {

// Forward-declare the upstream implementations so our override bodies can
// chain to them after the `#define` rebrand.
bool IsDecoderBuiltInVideoCodec_ChromiumImpl(VideoCodec codec);
bool IsDecoderSupportedVideoType_ChromiumImpl(const VideoType& type);
bool IsDefaultDecoderSupportedVideoType_ChromiumImpl(const VideoType& type);

}  // namespace media

#define IsDecoderBuiltInVideoCodec IsDecoderBuiltInVideoCodec_ChromiumImpl
#define IsDecoderSupportedVideoType IsDecoderSupportedVideoType_ChromiumImpl
#define IsDefaultDecoderSupportedVideoType \
  IsDefaultDecoderSupportedVideoType_ChromiumImpl

#include <media/base/supported_types.cc>

#undef IsDefaultDecoderSupportedVideoType
#undef IsDecoderSupportedVideoType
#undef IsDecoderBuiltInVideoCodec

namespace media {

bool FFmpegSupportsHEVC() {
#if BUILDFLAG(IS_LINUX) && BUILDFLAG(ENABLE_PLATFORM_HEVC)
  return base::FeatureList::IsEnabled(kFFmpegSoftwareHEVCDecoder);
#else
  return false;
#endif
}

VideoCodec HEVCAsH264IfFFmpegSupportsHEVC(VideoCodec codec) {
  if (FFmpegSupportsHEVC() && codec == VideoCodec::kHEVC) {
    return VideoCodec::kH264;
  }
  return codec;
}

bool IsDecoderBuiltInVideoCodec(VideoCodec codec) {
  if (FFmpegSupportsHEVC() && codec == VideoCodec::kHEVC) {
    return true;
  }
  return IsDecoderBuiltInVideoCodec_ChromiumImpl(codec);
}

// On Linux, upstream gates HEVC support on a hardware decoder registering its
// profiles via the supplemental profile cache (the PLATFORM_HAS_OPTIONAL_HEVC_
// DECODE_SUPPORT path in IsDecoderHevcProfileSupported). We ship a bundled
// FFmpeg HEVC software decoder, so advertise HEVC as supported when the
// upstream check rejects it for that reason. Both the default and public
// query functions are overridden so callers that go through the
// MediaClient-aware entry point also see HEVC as supported.
bool IsDefaultDecoderSupportedVideoType(const VideoType& type) {
  if (FFmpegSupportsHEVC() && type.codec == VideoCodec::kHEVC) {
    return true;
  }
  return IsDefaultDecoderSupportedVideoType_ChromiumImpl(type);
}

bool IsDecoderSupportedVideoType(const VideoType& type) {
  if (FFmpegSupportsHEVC() && type.codec == VideoCodec::kHEVC) {
    return true;
  }
  return IsDecoderSupportedVideoType_ChromiumImpl(type);
}

}  // namespace media
