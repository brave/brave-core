/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <random>

#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/modules/mediastream/media_device_info.h"

using blink::DynamicTo;
using blink::ExecutionContext;
using blink::LocalDOMWindow;
using blink::MediaDeviceInfoVector;
using blink::To;
using blink::WebContentSettingsClient;
using blink::WorkerGlobalScope;

namespace brave {

void FarbleMediaDevices(ExecutionContext* context,
                        MediaDeviceInfoVector* media_devices) {
  // |media_devices| is guaranteed not to be null here.
  if (media_devices->size() <= 2)
    return;
  WebContentSettingsClient* settings = GetContentSettingsClientFor(context);
  if (!settings)
    return;
  if (settings->GetBraveFarblingLevel() == BraveFarblingLevel::OFF)
    return;
  // Shuffle the list of devices pseudo-randomly, based on the
  // domain+session key, starting with the second device.
  std::mt19937_64 prng =
      BraveSessionCache::From(*context).MakePseudoRandomGenerator();
  MediaDeviceInfoVector::iterator it_begin = media_devices->begin();
  std::shuffle(++it_begin, media_devices->end(), prng);
}

}  // namespace brave

#define BRAVE_MEDIA_DEVICES_DEVICES_ENUMERATED                       \
  if (ExecutionContext* context = resolver->GetExecutionContext()) { \
    brave::FarbleMediaDevices(context, &media_devices);              \
  }

#include "../../../../../../../third_party/blink/renderer/modules/mediastream/media_devices.cc"
#undef BRAVE_MEDIA_DEVICES_DEVICES_ENUMERATED
