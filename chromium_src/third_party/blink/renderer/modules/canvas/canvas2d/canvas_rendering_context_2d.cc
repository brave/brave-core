/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl_helper.h"

#define BRAVE_CANVAS_RENDERING_CONTEXT_2D_MEASURE_TEXT          \
  if (!AllowFingerprinting(canvas()->GetDocument().GetFrame())) \
    return MakeGarbageCollected<TextMetrics>();

#include "../../../../../../../third_party/blink/renderer/modules/canvas/canvas2d/canvas_rendering_context_2d.cc"
#undef BRAVE_CANVAS_RENDERING_CONTEXT_2D_MEASURE_TEXT
