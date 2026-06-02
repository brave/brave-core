/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_origin;

import android.app.Activity;
import android.os.Bundle;

import org.jni_zero.CalledByNative;

import org.chromium.base.ApplicationStatus;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.settings.BraveOriginPreferences;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;

/**
 * Opens the Brave Origin settings screen from native when a purchase is first detected, so the user
 * can restart to apply the newly enforced policies.
 */
@NullMarked
public class BraveOriginSettingsLauncherHelper {
    @CalledByNative
    private static void showOriginSettingsForRestart() {
        Activity activity = ApplicationStatus.getLastTrackedFocusedActivity();
        if (activity == null) {
            return;
        }
        Bundle args = new Bundle();
        args.putBoolean(BraveOriginPreferences.EXTRA_SHOW_RESTART_PROMPT, true);
        SettingsNavigationFactory.createSettingsNavigation()
                .startSettings(activity, BraveOriginPreferences.class, args);
    }
}
