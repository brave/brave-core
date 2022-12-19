/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/modules/service_worker/service_worker_content_settings_proxy.cc"

namespace blink {

BraveFarblingLevel ServiceWorkerContentSettingsProxy::GetBraveFarblingLevel() {
  uint8_t result = BraveFarblingLevel::OFF;
  GetService()->GetBraveFarblingLevel(&result);
  if (result == 0)
    return BraveFarblingLevel::BALANCED;
  else if (result == 1)
    return BraveFarblingLevel::OFF;
  return BraveFarblingLevel::MAXIMUM;
}

}  // namespace blink
