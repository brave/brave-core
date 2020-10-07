/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/webaudio/analyser_node.h"

#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"

#define BRAVE_ANALYSERHANDLER_CONSTRUCTOR                                  \
  if (ExecutionContext* context = node.GetExecutionContext()) {         \
    WebContentSettingsClient* settings = nullptr;                       \
    if (auto* window = DynamicTo<LocalDOMWindow>(context))              \
      settings = window->GetFrame()->GetContentSettingsClient();        \
    else if (context->IsWorkerGlobalScope())                            \
      settings = To<WorkerGlobalScope>(context)->ContentSettingsClient(); \
    analyser_.audio_farbling_callback_ =                                \
        brave::BraveSessionCache::From(*context).GetAudioFarblingCallback( \
            settings);                                                     \
  }

#include "../../../../../../../third_party/blink/renderer/modules/webaudio/analyser_node.cc"

#undef BRAVE_ANALYSERNODE_CONSTRUCTOR
