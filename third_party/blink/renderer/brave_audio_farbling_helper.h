/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_AUDIO_FARBLING_HELPER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_AUDIO_FARBLING_HELPER_H_

#include <stddef.h>
#include <stdint.h>

#include "third_party/blink/public/platform/web_common.h"

namespace brave {

class BLINK_EXPORT AudioFarblingHelper {
 public:
  AudioFarblingHelper(double fudge_factor, uint64_t seed);
  ~AudioFarblingHelper();

  void SetMax(bool max) { max_ = max; }
  void FarbleAudioChannel(float* dst, size_t count);
  void FarbleFloatTimeDomainData(float* input_buffer,
                                 float* destination,
                                 size_t len,
                                 unsigned write_index,
                                 unsigned fft_size,
                                 unsigned input_buffer_size);
  void FarbleByteTimeDomainData(float* input_buffer,
                                unsigned char* destination,
                                size_t len,
                                unsigned write_index,
                                unsigned fft_size,
                                unsigned input_buffer_size);
  void FarbleConvertToByteData(const float* source,
                               unsigned char* destination,
                               size_t len,
                               const double min_decibels,
                               const double range_scale_factor);
  void FarbleConvertFloatToDb(const float* source,
                              float* destination,
                              size_t len);

 private:
  double fudge_factor_;
  uint64_t seed_;
  bool max_;
};

}  // namespace brave

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_BRAVE_AUDIO_FARBLING_HELPER_H_
