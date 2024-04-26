/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/wdp_service.h"
#include "components/prefs/pref_service.h"

namespace web_discovery {

WDPService::WDPService(PrefService* profile_prefs)
    : profile_prefs_(profile_prefs) {}

}  // namespace web_discovery
