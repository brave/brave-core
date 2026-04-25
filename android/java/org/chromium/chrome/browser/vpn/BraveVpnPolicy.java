/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

/** Provides policy state for Brave VPN. */
@NullMarked
public class BraveVpnPolicy {
    /** Returns true if VPN is disabled by policy for the given profile. */
    public static boolean isDisabledByPolicy(@Nullable Profile profile) {
        if (profile == null) {
            return false;
        }
        PrefService prefService = UserPrefs.get(profile);
        return prefService.isManagedPreference(BravePref.MANAGED_BRAVE_VPN_DISABLED)
                && prefService.getBoolean(BravePref.MANAGED_BRAVE_VPN_DISABLED);
    }
}
