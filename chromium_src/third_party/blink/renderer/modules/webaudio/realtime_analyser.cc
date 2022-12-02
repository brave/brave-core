/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_REALTIMEANALYSER_CONVERTFLOATTODB                   \
  if (convert_float_to_db_callback_) {                            \
    convert_float_to_db_callback_->Run(source, destination, len); \
    return;                                                       \
  }

#define BRAVE_REALTIMEANALYSER_CONVERTTOBYTEDATA                           \
  if (convert_to_byte_data_callback_) {                                    \
    convert_to_byte_data_callback_->Run(source, destination, len,          \
                                        min_decibels, range_scale_factor); \
    return;                                                                \
  }

#define BRAVE_REALTIMEANALYSER_GETFLOATTIMEDOMAINDATA                     \
  if (float_time_domain_data_callback_) {                                 \
    float_time_domain_data_callback_->Run(input_buffer, destination, len, \
                                          write_index, fft_size,          \
                                          kInputBufferSize);              \
    return;                                                               \
  }

#define BRAVE_REALTIMEANALYSER_GETBYTETIMEDOMAINDATA                     \
  if (byte_time_domain_data_callback_) {                                 \
    byte_time_domain_data_callback_->Run(input_buffer, destination, len, \
                                         write_index, fft_size,          \
                                         kInputBufferSize);              \
    return;                                                              \
  }

#include "src/third_party/blink/renderer/modules/webaudio/realtime_analyser.cc"

#undef BRAVE_REALTIMEANALYSER_CONVERTFLOATTODB
#undef BRAVE_REALTIMEANALYSER_CONVERTTOBYTEDATA
#undef BRAVE_REALTIMEANALYSER_GETFLOATTIMEDOMAINDATA
#undef BRAVE_REALTIMEANALYSER_GETBYTETIMEDOMAINDATA
