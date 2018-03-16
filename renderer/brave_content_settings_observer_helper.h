/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_RENDERER_BRAVE_CONTENT_SETTINGS_OBSERVER_HELPER_H_
#define BRAVE_RENDERER_BRAVE_CONTENT_SETTINGS_OBSERVER_HELPER_H_

#include "core/frame/ContentSettingsClient.h"
#include "core/frame/LocalFrame.h"

static bool AllowFingerprinting(blink::LocalFrame* frame) {
  if (!frame) return true;
  return frame->GetContentSettingsClient()->AllowFingerprinting(true);
}

#endif
