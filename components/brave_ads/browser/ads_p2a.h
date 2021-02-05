/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_P2A_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_P2A_H_

#include <stdint.h>

#include <string>
#include <vector>

class PrefService;
class PrefRegistrySimple;

namespace brave_ads {

void RegisterP2APrefs(PrefRegistrySimple* prefs);

void RecordInWeeklyStorageAndEmitP2AHistogramAnswer(PrefService* prefs,
                                                    const std::string& name);

void EmitP2AHistogramAnswer(const std::string& name, uint16_t count_value);

void SuspendP2AHistograms();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_P2A_H_
