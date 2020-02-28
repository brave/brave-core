/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/modules/webaudio/analyser_node.h"

#define BRAVE_AUDIOBUFFER_GETCHANNELDATA                            \
  NotShared<DOMFloat32Array> array = getChannelData(channel_index); \
  LocalDOMWindow* window = LocalDOMWindow::From(script_state);      \
  if (window) {                                                     \
    DOMFloat32Array* destination_array = array.View();              \
    size_t len = destination_array->lengthAsSizeT();                \
    if (len > 0) {                                                  \
      float* destination = destination_array->Data();               \
      double fudge_factor =                                         \
          brave::BraveSessionCache::From(*(window->document()))     \
              .GetFudgeFactor();                                    \
      for (unsigned i = 0; i < len; ++i) {                          \
        destination[i] = destination[i] * fudge_factor;             \
      }                                                             \
      return array;                                                 \
    }                                                               \
  }

#define BRAVE_AUDIOBUFFER_COPYFROMCHANNEL                             \
  LocalDOMWindow* window = LocalDOMWindow::From(script_state);        \
  if (window) {                                                       \
    double fudge_factor =                                             \
        brave::BraveSessionCache::From(*(window->document()))         \
            .GetFudgeFactor();                                        \
    for (unsigned i = 0; i < count; ++i) {                            \
      dst[i + buffer_offset] = dst[i + buffer_offset] * fudge_factor; \
    }                                                                 \
  }

#include "../../../../../../third_party/blink/renderer/modules/webaudio/audio_buffer.cc"

#undef BRAVE_AUDIOBUFFER_GETCHANNELDATA
#undef BRAVE_AUDIOBUFFER_COPYFROMCHANNEL
