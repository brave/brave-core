/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/ABC): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/third_party/blink/renderer/platform/brave_audio_farbling_helper.h"

#include <limits.h>

#include "third_party/blink/renderer/platform/audio/audio_utilities.h"

namespace blink {
namespace {

constexpr uint64_t zero = 0;
constexpr double maxUInt64AsDouble = static_cast<double>(UINT64_MAX);

inline uint64_t lfsr_next(uint64_t v) {
  return ((v >> 1) | (((v << 62) ^ (v << 61)) & (~(~zero << 63) << 62)));
}

}  // namespace

BraveAudioFarblingHelper::BraveAudioFarblingHelper(double fudge_factor,
                                                   uint64_t seed,
                                                   bool max)
    : fudge_factor_(fudge_factor), seed_(seed), max_(max) {}

BraveAudioFarblingHelper::~BraveAudioFarblingHelper() = default;

void BraveAudioFarblingHelper::FarbleAudioChannel(float* dst,
                                                  size_t count) const {
  if (max_) {
    uint64_t v = seed_;
    for (size_t i = 0; i < count; i++) {
      v = lfsr_next(v);
      dst[i] = (v / maxUInt64AsDouble) / 10;
    }
  } else {
    for (size_t i = 0; i < count; i++) {
      dst[i] = dst[i] * fudge_factor_;
    }
  }
}

// Calculate values for RealtimeAnalyser::GetFloatTimeDomainData
void BraveAudioFarblingHelper::FarbleFloatTimeDomainData(
    const float* input_buffer,
    float* destination,
    size_t len,
    unsigned write_index,
    unsigned fft_size,
    unsigned input_buffer_size) const {
  if (max_) {
    uint64_t v = seed_;
    for (size_t i = 0; i < len; ++i) {
      v = lfsr_next(v);
      float value = (v / maxUInt64AsDouble) / 10;
      destination[i] = value;
    }
  } else {
    for (size_t i = 0; i < len; ++i) {
      // Buffer access is protected due to modulo operation.
      float value =
          fudge_factor_ *
          input_buffer[(i + write_index - fft_size + input_buffer_size) %
                       input_buffer_size];

      destination[i] = value;
    }
  }
}

// Calculate values for RealtimeAnalyser::GetByteTimeDomainData
void BraveAudioFarblingHelper::FarbleByteTimeDomainData(
    const float* input_buffer,
    unsigned char* destination,
    size_t len,
    unsigned write_index,
    unsigned fft_size,
    unsigned input_buffer_size) const {
  if (max_) {
    uint64_t v = seed_;
    for (size_t i = 0; i < len; ++i) {
      v = lfsr_next(v);
      float value = (v / maxUInt64AsDouble) / 10;

      // Scale from nominal -1 -> +1 to unsigned byte.
      double scaled_value = 128 * (value + 1);

      // Clip to valid range.
      if (scaled_value < 0) {
        scaled_value = 0;
      }
      if (scaled_value > UCHAR_MAX) {
        scaled_value = UCHAR_MAX;
      }

      destination[i] = static_cast<unsigned char>(scaled_value);
    }
  } else {
    for (size_t i = 0; i < len; ++i) {
      // Buffer access is protected due to modulo operation.
      float value =
          fudge_factor_ *
          input_buffer[(i + write_index - fft_size + input_buffer_size) %
                       input_buffer_size];

      // Scale from nominal -1 -> +1 to unsigned byte.
      double scaled_value = 128 * (value + 1);

      // Clip to valid range.
      if (scaled_value < 0) {
        scaled_value = 0;
      }
      if (scaled_value > UCHAR_MAX) {
        scaled_value = UCHAR_MAX;
      }

      destination[i] = static_cast<unsigned char>(scaled_value);
    }
  }
}

// Calculate values for RealtimeAnalyser::ConvertToByteData
void BraveAudioFarblingHelper::FarbleConvertToByteData(
    const float* source,
    unsigned char* destination,
    size_t len,
    double min_decibels,
    double range_scale_factor) const {
  if (max_) {
    uint64_t v = seed_;
    for (size_t i = 0; i < len; ++i) {
      v = lfsr_next(v);
      float linear_value = (v / maxUInt64AsDouble) / 10;
      double db_mag = audio_utilities::LinearToDecibels(linear_value);

      // The range m_minDecibels to m_maxDecibels will be scaled to byte values
      // from 0 to UCHAR_MAX.
      double scaled_value =
          UCHAR_MAX * (db_mag - min_decibels) * range_scale_factor;

      // Clip to valid range.
      if (scaled_value < 0) {
        scaled_value = 0;
      }
      if (scaled_value > UCHAR_MAX) {
        scaled_value = UCHAR_MAX;
      }

      destination[i] = static_cast<unsigned char>(scaled_value);
    }
  } else {
    for (size_t i = 0; i < len; ++i) {
      float linear_value = fudge_factor_ * source[i];
      double db_mag = audio_utilities::LinearToDecibels(linear_value);

      // The range m_minDecibels to m_maxDecibels will be scaled to byte values
      // from 0 to UCHAR_MAX.
      double scaled_value =
          UCHAR_MAX * (db_mag - min_decibels) * range_scale_factor;

      // Clip to valid range.
      if (scaled_value < 0) {
        scaled_value = 0;
      }
      if (scaled_value > UCHAR_MAX) {
        scaled_value = UCHAR_MAX;
      }

      destination[i] = static_cast<unsigned char>(scaled_value);
    }
  }
}

// Calculate values for RealtimeAnalyser::ConvertFloatToDb
void BraveAudioFarblingHelper::FarbleConvertFloatToDb(const float* source,
                                                      float* destination,
                                                      size_t len) const {
  if (max_) {
    uint64_t v = seed_;
    for (size_t i = 0; i < len; ++i) {
      v = lfsr_next(v);
      float linear_value = (v / maxUInt64AsDouble) / 10;
      double db_mag = audio_utilities::LinearToDecibels(linear_value);
      destination[i] = static_cast<float>(db_mag);
    }
  } else {
    for (size_t i = 0; i < len; ++i) {
      float linear_value = fudge_factor_ * source[i];
      double db_mag = audio_utilities::LinearToDecibels(linear_value);
      destination[i] = static_cast<float>(db_mag);
    }
  }
}

}  // namespace blink
