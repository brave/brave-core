/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_RENDERER_BRAVE_CONTENT_SETTINGS_AGENT_IMPL_FARBLING_HELPER_H_
#define BRAVE_RENDERER_BRAVE_CONTENT_SETTINGS_AGENT_IMPL_FARBLING_HELPER_H_

#include "brave/common/brave_farbling_constants.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

static BraveFarblingLevel GetBraveFarblingLevel(blink::LocalFrame* frame) {
  if (!frame || !frame->GetContentSettingsClient()) {
    return BraveFarblingLevel::OFF;
  }

  return frame->GetContentSettingsClient()->GetBraveFarblingLevel();
}

#endif  // BRAVE_RENDERER_BRAVE_CONTENT_SETTINGS_AGENT_IMPL_FARBLING_HELPER_H_
