/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.build.annotations.NullMarked;

/**
 * Holds the cached policy state for Brave Rewards.
 *
 * <p>The cached value is updated every time BraveActivity is foregrounded (in onResumeWithNative).
 * This ensures the value is fresh when user returns from Settings where the policy could have been
 * changed.
 */
@NullMarked
public class BraveRewardsPolicy {
    private static boolean sDisabledByPolicy;

    /**
     * Set by BraveActivity.onResumeWithNative() when Rewards is disabled by Brave Origin policy.
     */
    public static void setDisabledByPolicy(boolean disabled) {
        sDisabledByPolicy = disabled;
    }

    /** Returns true if Rewards is disabled by Brave Origin policy. */
    public static boolean isDisabledByPolicy() {
        return sDisabledByPolicy;
    }
}
