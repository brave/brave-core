/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/common/util.h"

#include "brave/components/constants/pref_names.h"
#include "components/prefs/pref_service.h"

namespace web_discovery {

bool IsWebDiscoveryEnabled(PrefService& pref_service) {
  return pref_service.GetBoolean(kWebDiscoveryEnabled) &&
         !pref_service.GetBoolean(kWebDiscoveryDisabledByPolicy);
}

}  // namespace web_discovery
