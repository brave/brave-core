/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/modules/service_worker/service_worker_content_settings_proxy.cc"

#include "brave/components/brave_shields/core/common/shields_settings.mojom-blink.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"

namespace blink {

brave_shields::mojom::ShieldsSettingsPtr
ServiceWorkerContentSettingsProxy::GetBraveShieldsSettings(
    ContentSettingsType webcompat_settings_type) {
  brave_shields::mojom::blink::ShieldsSettingsPtr blink_result;
  if (!GetService()->GetBraveShieldsSettings(&blink_result)) {
    return brave_shields::mojom::ShieldsSettings::New();
  }

  // Convert the blink mojo struct into a non-blink mojo struct.
  CHECK(blink_result);
  brave_shields::mojom::ShieldsSettingsPtr result;
  CHECK(brave_shields::mojom::ShieldsSettings::DeserializeFromMessage(
      brave_shields::mojom::blink::ShieldsSettings::WrapAsMessage(
          std::move(blink_result)),
      &result));
  return result;
}

}  // namespace blink
