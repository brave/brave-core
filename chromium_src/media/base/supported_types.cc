/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "media/media_buildflags.h"

#define IsDecoderBuiltInVideoCodec IsDecoderBuiltInVideoCodec_ChromiumImpl

#include <media/base/supported_types.cc>

#undef IsDecoderBuiltInVideoCodec

namespace media {

MEDIA_EXPORT bool IsDecoderBuiltInVideoCodec(VideoCodec codec) {
#if BUILDFLAG(IS_LINUX) && BUILDFLAG(ENABLE_PLATFORM_HEVC)
  if (codec == VideoCodec::kHEVC)
    return true;
#endif
  return IsDecoderBuiltInVideoCodec_ChromiumImpl(codec);
}

}  // namespace media
