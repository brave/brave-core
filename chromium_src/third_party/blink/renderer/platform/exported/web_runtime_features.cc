/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../../../../third_party/blink/renderer/platform/exported/web_runtime_features.cc"

namespace blink {

void WebRuntimeFeatures::EnableNavigatorPluginsFixed(bool enable) {
  RuntimeEnabledFeatures::SetNavigatorPluginsFixedEnabled(enable);
}

}  // namespace blink
