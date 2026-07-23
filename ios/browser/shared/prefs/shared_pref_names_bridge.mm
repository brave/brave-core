// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/shared/prefs/shared_pref_names_bridge.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/shared/prefs/pref_names.h"
#include "components/metrics/metrics_pref_names.h"

NSString* const kMediaBackgroundingEnabled =
    base::SysUTF8ToNSString(prefs::kMediaBackgroundingEnabled);

NSString* const kBlockAllCookiesEnabled =
    base::SysUTF8ToNSString(prefs::kBlockAllCookiesEnabled);

// Also used by `chrome://crashes` and `IOSChromeMainParts` to gate
// `MetricsServicesManager` upload permissions; kept in sync with crash report
// consent in `BraveCoreMain` rather than introducing a separate pref.
NSString* const kMetricsReportingEnabled =
    base::SysUTF8ToNSString(metrics::prefs::kMetricsReportingEnabled);
