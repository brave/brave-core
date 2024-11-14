/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_PLATFORM_BRAVE_AUDIO_FARBLING_HELPER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_PLATFORM_BRAVE_AUDIO_FARBLING_HELPER_H_

#include <stddef.h>
#include <stdint.h>

#include "base/containers/span.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class PLATFORM_EXPORT BraveAudioFarblingHelper final {
 public:
  BraveAudioFarblingHelper(double fudge_factor, uint64_t seed, bool max);
  ~BraveAudioFarblingHelper();

  void FarbleAudioChannel(base::span<float> dst) const;
  void FarbleFloatTimeDomainData(const float* input_buffer,
                                 float* destination,
                                 size_t len,
                                 unsigned write_index,
                                 unsigned fft_size,
                                 unsigned input_buffer_size) const;
  void FarbleByteTimeDomainData(const float* input_buffer,
                                unsigned char* destination,
                                size_t len,
                                unsigned write_index,
                                unsigned fft_size,
                                unsigned input_buffer_size) const;
  void FarbleConvertToByteData(const float* source,
                               unsigned char* destination,
                               size_t len,
                               double min_decibels,
                               double range_scale_factor) const;
  void FarbleConvertFloatToDb(const float* source,
                              float* destination,
                              size_t len) const;

 private:
  double fudge_factor_;
  uint64_t seed_;
  bool max_;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_PLATFORM_BRAVE_AUDIO_FARBLING_HELPER_H_
