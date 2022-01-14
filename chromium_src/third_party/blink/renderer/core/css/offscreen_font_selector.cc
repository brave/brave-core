/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/css/offscreen_font_selector.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"

#include "src/third_party/blink/renderer/core/css/offscreen_font_selector.cc"

namespace blink {

bool OffscreenFontSelector::AllowFontFamily(const AtomicString& family_name) {
  if (ExecutionContext* context = GetExecutionContext()) {
    if (WebContentSettingsClient* settings =
            brave::GetContentSettingsClientFor(context)) {
      return brave::BraveSessionCache::From(*context).AllowFontFamily(
          settings, family_name);
    }
  }
  return true;
}

}  // namespace blink
