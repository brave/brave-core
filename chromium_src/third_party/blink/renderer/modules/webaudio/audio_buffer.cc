/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/functional/callback.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/modules/webaudio/analyser_node.h"

#define BRAVE_AUDIOBUFFER_GETCHANNELDATA                                      \
  {                                                                           \
    NotShared<DOMFloat32Array> destination = getChannelData(channel_index);   \
    base::span<float> dst = destination->AsSpan();                            \
    if (!dst.empty()) {                                                       \
      if (ExecutionContext* context = ExecutionContext::From(script_state)) { \
        brave::BraveSessionCache::From(*context).FarbleAudioChannel(dst);     \
      }                                                                       \
    }                                                                         \
  }

#define BRAVE_AUDIOBUFFER_COPYFROMCHANNEL                                 \
  if (ExecutionContext* context = ExecutionContext::From(script_state)) { \
    brave::BraveSessionCache::From(*context).FarbleAudioChannel(          \
        dst.first(count));                                                \
  }

#include "src/third_party/blink/renderer/modules/webaudio/audio_buffer.cc"

#undef BRAVE_AUDIOBUFFER_GETCHANNELDATA
#undef BRAVE_AUDIOBUFFER_COPYFROMCHANNEL
