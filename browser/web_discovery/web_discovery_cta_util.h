/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_CTA_UTIL_H_
#define BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_CTA_UTIL_H_

#include <string>

#include "base/time/time.h"

class GURL;
class PrefService;
class TemplateURLService;

namespace base {
class Clock;
}  // namespace base

// Stores the current cta's state.
struct WebDiscoveryCTAState {
  std::string id;
  // The number of times Infobar shown so far.
  int count = 0;
  bool dismissed = false;
  base::Time last_displayed;
};

WebDiscoveryCTAState GetWebDiscoveryCTAState(PrefService* prefs,
                                             const std::string& cta_id);
void SetWebDiscoveryCTAStateToPrefs(PrefService* prefs,
                                    const WebDiscoveryCTAState& state);
std::string GetWebDiscoveryCurrentCTAId();
bool ShouldShowWebDiscoveryInfoBar(TemplateURLService* service,
                                   PrefService* prefs,
                                   const WebDiscoveryCTAState& state,
                                   base::Clock* test_clock = nullptr);
std::string& GetWebDiscoveryCTAIDForTesting();

#endif  // BRAVE_BROWSER_WEB_DISCOVERY_WEB_DISCOVERY_CTA_UTIL_H_
