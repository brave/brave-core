// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.safe_browsing.settings;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.chrome.browser.safe_browsing.SafeBrowsingBridge;

/**
 * Fragment containing standard protection settings.
 */
public class BraveStandardProtectionSettingsFragment extends StandardProtectionSettingsFragment {
    public void updateLeakDetectionAndExtendedReportingPreferences() {
        BraveReflectionUtil.InvokeMethod(StandardProtectionSettingsFragment.class, this,
                "updateLeakDetectionAndExtendedReportingPreferences");

        SafeBrowsingBridge.setSafeBrowsingExtendedReportingEnabled(false);
        getPreferenceScreen().removePreference(mExtendedReportingPreference);
        getPreferenceScreen().removePreference(mPasswordLeakDetectionPreference);
    }
}
