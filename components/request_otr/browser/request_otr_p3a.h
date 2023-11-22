/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_P3A_H_
#define BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_P3A_H_

#include "base/time/time.h"

class PrefRegistrySimple;
class PrefService;

namespace request_otr {
namespace p3a {

inline constexpr char kSessionCountHistogramName[] =
    "Brave.RequestOTR.SessionCount";
inline constexpr char kInterstitialDurationHistogramName[] =
    "Brave.RequestOTR.InterstitialDuration";
inline constexpr char kInterstitialShownHistogramName[] =
    "Brave.RequestOTR.InterstitialShown";

void RegisterProfilePrefs(PrefRegistrySimple* registry);

void RecordSessionCount(PrefService* prefs, bool new_session_started);

void RecordInterstitialShown(PrefService* prefs, bool new_page_shown);
void RecordInterstitialEnd(PrefService* prefs, base::Time new_page_start_time);

void UpdateMetrics(PrefService* prefs);

}  // namespace p3a
}  // namespace request_otr

#endif  // BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_P3A_H_
