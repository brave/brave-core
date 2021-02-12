/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"

#define BRAVE_TO_DATA_URL_INTERNAL                                          \
  if (ExecutionContext* context = GetExecutionContext()) {                  \
    if (WebContentSettingsClient* settings =                                \
            brave::GetContentSettingsClientFor(context)) {                  \
      brave::BraveSessionCache::From(*context).PerturbPixels(               \
          settings, data_buffer->Pixels(), data_buffer->ComputeByteSize()); \
    }                                                                       \
  }

#include "../../../../../../../../third_party/blink/renderer/core/html/canvas/html_canvas_element.cc"

#undef BRAVE_TO_DATA_URL_INTERNAL
