/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define SetRuntimeFeaturesDefaultsAndUpdateFromArgs \
  SetRuntimeFeaturesDefaultsAndUpdateFromArgs_ChromiumImpl

#include "../../../../content/child/runtime_features.cc"

#undef SetRuntimeFeaturesDefaultsAndUpdateFromArgs

namespace content {

void SetRuntimeFeaturesDefaultsAndUpdateFromArgs(
    const base::CommandLine& command_line) {
  SetRuntimeFeaturesDefaultsAndUpdateFromArgs_ChromiumImpl(command_line);

  // Brave will use its own mechanism for farbling the actual list of plugins
  // returned by navigator.plugins depending on the selected BraveFarblingLevel.
  WebRuntimeFeatures::EnableNavigatorPluginsFixed(false);
}

}  // namespace content
