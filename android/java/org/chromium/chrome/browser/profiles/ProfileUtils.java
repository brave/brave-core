/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.profiles;

import org.chromium.base.Log;
import org.chromium.chrome.browser.app.BraveActivity;

public class ProfileUtils {
    private static final String TAG = "ProfileUtils";

    public static Profile getProfile() {
        BraveActivity braveActivity = null;
        try {
            braveActivity = BraveActivity.getBraveActivity();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "getProfile " + e);
        }
        if (braveActivity == null) return ProfileManager.getLastUsedRegularProfile();
        return braveActivity.getCurrentProfile();
    }
}
