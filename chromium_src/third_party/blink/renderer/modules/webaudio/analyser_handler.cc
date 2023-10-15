/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"

#define BRAVE_ANALYSERHANDLER_CONSTRUCTOR                                   \
  if (ExecutionContext* context = node.GetExecutionContext()) {             \
    analyser_.set_audio_farbling_helper(                                    \
        brave::BraveSessionCache::From(*context).GetAudioFarblingHelper()); \
  }

#include "src/third_party/blink/renderer/modules/webaudio/analyser_handler.cc"
#undef BRAVE_ANALYSERHANDLER_CONSTRUCTOR
