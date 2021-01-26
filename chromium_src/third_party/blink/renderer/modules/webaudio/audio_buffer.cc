/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/callback.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/modules/webaudio/analyser_node.h"

#define BRAVE_AUDIOBUFFER_GETCHANNELDATA                                       \
  NotShared<DOMFloat32Array> array = getChannelData(channel_index);            \
  if (ExecutionContext* context = ExecutionContext::From(script_state)) {      \
    if (WebContentSettingsClient* settings =                                   \
            brave::GetContentSettingsClientFor(context)) {                     \
      DOMFloat32Array* destination_array = array.Get();                       \
      size_t len = destination_array->length();                                \
      if (len > 0) {                                                           \
        float* destination = destination_array->Data();                        \
        brave::AudioFarblingCallback audio_farbling_callback =                 \
            brave::BraveSessionCache::From(*context).GetAudioFarblingCallback( \
                settings);                                                     \
        for (unsigned i = 0; i < len; ++i) {                                   \
          destination[i] = audio_farbling_callback.Run(destination[i], i);     \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }

#define BRAVE_AUDIOBUFFER_COPYFROMCHANNEL                                    \
  if (ExecutionContext* context = ExecutionContext::From(script_state)) {    \
    if (WebContentSettingsClient* settings =                                 \
            brave::GetContentSettingsClientFor(context)) {                   \
      brave::AudioFarblingCallback audio_farbling_callback =                 \
          brave::BraveSessionCache::From(*context).GetAudioFarblingCallback( \
              settings);                                                     \
      for (unsigned i = 0; i < count; i++) {                                 \
        dst[i] = audio_farbling_callback.Run(dst[i], i);                     \
      }                                                                      \
    }                                                                        \
  }

#include "../../../../../../../third_party/blink/renderer/modules/webaudio/audio_buffer.cc"

#undef BRAVE_AUDIOBUFFER_GETCHANNELDATA
#undef BRAVE_AUDIOBUFFER_COPYFROMCHANNEL
