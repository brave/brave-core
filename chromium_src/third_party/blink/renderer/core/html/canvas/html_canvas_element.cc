/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"

#define BRAVE_TO_DATA_URL_INTERNAL                                           \
  if (ExecutionContext* context = GetExecutionContext()) {                   \
    if (WebContentSettingsClient* settings =                                 \
            brave::GetContentSettingsClientFor(context)) {                   \
      image_bitmap = brave::BraveSessionCache::From(*context).PerturbPixels( \
          settings, image_bitmap);                                           \
      if (!image_bitmap)                                                     \
        return String("data:,");                                             \
    }                                                                        \
  }

#include "../../../../../../../../third_party/blink/renderer/core/html/canvas/html_canvas_element.cc"

#undef BRAVE_TO_DATA_URL_INTERNAL
