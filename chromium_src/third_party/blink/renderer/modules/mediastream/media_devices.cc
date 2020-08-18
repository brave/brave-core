/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <random>

#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/mediastream/media_device_info.h"

namespace brave {

void FarbleMediaDevices(blink::LocalFrame* frame,
                        blink::MediaDeviceInfoVector* media_devices) {
  if (blink::WebContentSettingsClient* settings =
          frame->GetContentSettingsClient()) {
    if (settings->GetBraveFarblingLevel() != BraveFarblingLevel::OFF) {
      // Shuffle the list of plugins pseudo-randomly, based on the
      // domain+session key.
      std::mt19937_64 prng = BraveSessionCache::From(*(frame->GetDocument()))
                                 .MakePseudoRandomGenerator();
      std::shuffle(media_devices->begin(), media_devices->end(), prng);
    }
  }
}

}  // namespace brave

#define BRAVE_MEDIA_DEVICES_DEVICES_ENUMERATED                         \
  if (ScriptState* script_state = resolver->GetScriptState()) {        \
    if (LocalDOMWindow* window = LocalDOMWindow::From(script_state)) { \
      if (LocalFrame* frame = window->GetFrame()) {                    \
        brave::FarbleMediaDevices(frame, &media_devices);              \
      }                                                                \
    }                                                                  \
  }

#include "../../../../../../../third_party/blink/renderer/modules/mediastream/media_devices.cc"
#undef BRAVE_MEDIA_DEVICES_DEVICES_ENUMERATED
