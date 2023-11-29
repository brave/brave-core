/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import org.chromium.base.Log;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.user_prefs.UserPrefs;

public class BraveLeoPrefUtils {
    private static final String TAG = "BraveLeoPrefUtils";

    public static void setIsSubscriptionActive(boolean value) {
        Profile profile = null;
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            profile = activity.getCurrentProfile();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "get BraveActivity exception", e);
        }
        if (profile == null) {
            Log.e(TAG, "setIsSubscriptionActive profile is null");
            return;
        }
        UserPrefs.get(profile).setBoolean(BravePref.BRAVE_CHAT_SUBSCRIPTION_ACTIVE_ANDROID, value);
    }
}
