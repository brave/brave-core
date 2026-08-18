/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.ResettersForTesting;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

/** Provides policy state for Brave Rewards. */
@NullMarked
public class BraveRewardsPolicy {
    private static @Nullable Boolean sDisabledByPolicyForTesting;

    /** Returns true if Rewards is disabled by policy for the given profile. */
    public static boolean isDisabledByPolicy(@Nullable Profile profile) {
        if (sDisabledByPolicyForTesting != null) {
            return sDisabledByPolicyForTesting;
        }
        if (profile == null) {
            return false;
        }
        PrefService prefService = UserPrefs.get(profile);
        return prefService.isManagedPreference(BravePref.DISABLED_BY_POLICY)
                && prefService.getBoolean(BravePref.DISABLED_BY_POLICY);
    }

    /** Overrides the policy state in tests. Automatically reset after the test finishes. */
    public static void setDisabledByPolicyForTesting(boolean disabled) {
        sDisabledByPolicyForTesting = disabled;
        ResettersForTesting.register(() -> sDisabledByPolicyForTesting = null);
    }
}
