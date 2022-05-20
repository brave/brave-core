/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.firstrun;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.Nullable;

import org.chromium.chrome.browser.firstrun.WelcomeOnboardingActivity;

/**
 * Brave extension of FreIntentCreator.
 */
public class BraveFreIntentCreator extends FreIntentCreator {
    @Override
    protected Intent createInternal(Context caller, Intent fromIntent, boolean preferLightweightFre,
            @Nullable String associatedAppName) {
        // Launch WelcomeOnboardingActivity directly which is the only one enabled
        Intent welcomeOnboardingIntent = new Intent(caller, WelcomeOnboardingActivity.class);
        welcomeOnboardingIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        return welcomeOnboardingIntent;
    }
}
