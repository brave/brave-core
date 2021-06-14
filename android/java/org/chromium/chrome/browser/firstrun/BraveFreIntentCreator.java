/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.firstrun;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;

import androidx.annotation.Nullable;

import org.chromium.base.IntentUtils;
import org.chromium.chrome.browser.BraveConfig;
import org.chromium.chrome.browser.browserservices.intents.BrowserServicesIntentDataProvider;
import org.chromium.chrome.browser.firstrun.P3aOnboardingActivity;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;

/**
 * Brave extension of FreIntentCreator.
 */
public class BraveFreIntentCreator extends FreIntentCreator {
    @Override
    protected Intent createInternal(Context caller, Intent fromIntent, boolean preferLightweightFre,
            @Nullable String associatedAppName) {
        // Launch P3aOnboardingActivity directly which is the only one enabled
        Intent p3aOnboardingIntent = new Intent(caller, P3aOnboardingActivity.class);
        p3aOnboardingIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        return p3aOnboardingIntent;
    }
}
