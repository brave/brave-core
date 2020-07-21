/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <random>

#include "base/system/sys_info.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/navigator_concurrent_hardware.h"

namespace brave {

using blink::LocalFrame;

const unsigned kFakeMinProcessors = 2;
const unsigned kFakeMaxProcessors = 8;

unsigned FarbleNumberOfProcessors(LocalFrame* frame) {
  unsigned true_value =
      static_cast<unsigned>(base::SysInfo::NumberOfProcessors());
  if ((true_value <= 2) || !frame || !frame->GetContentSettingsClient())
    return true_value;
  unsigned farbled_value = true_value;
  switch (frame->GetContentSettingsClient()->GetBraveFarblingLevel()) {
    case BraveFarblingLevel::OFF: {
      break;
    }
    case BraveFarblingLevel::MAXIMUM: {
      true_value = kFakeMaxProcessors;
      // "Maximum" behavior is "balanced" behavior but with a fake maximum,
      // so fall through here.
      U_FALLTHROUGH;
    }
    case BraveFarblingLevel::BALANCED: {
      std::mt19937_64 prng = BraveSessionCache::From(*(frame->GetDocument()))
                                 .MakePseudoRandomGenerator();
      farbled_value =
          kFakeMinProcessors + (prng() % (true_value + 1 - kFakeMinProcessors));
      break;
    }
    default:
      NOTREACHED();
  }
  return farbled_value;
}

}  // namespace brave

namespace blink {

unsigned NavigatorConcurrentHardware::hardwareConcurrency(
    ScriptState* script_state) const {
  LocalFrame* frame = nullptr;
  if (LocalDOMWindow* window = LocalDOMWindow::From(script_state))
    frame = window->GetFrame();
  return brave::FarbleNumberOfProcessors(frame);
}

}  // namespace blink
