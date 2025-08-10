// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_manager.settings;

import android.os.Bundle;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.ApplicationTestUtils;
import org.chromium.chrome.browser.password_manager.BravePasswordManagerHelper;
import org.chromium.chrome.browser.password_manager.ManagePasswordsReferrer;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.settings.SettingsActivity;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;

/** Helper functions used in various password settings test suites. */
class PasswordSettingsTestHelper {
    private SettingsActivity mActivityToCleanUp;

    void tearDown() {
        try {
            ApplicationTestUtils.finishActivity(mActivityToCleanUp);
        } catch (Exception e) {
            // Activity was already finished by test framework. Any exception is not test-related.
        }
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    if (!ProfileManager.isInitialized()) return;
                    PasswordManagerHandlerProvider.getForProfile(
                                    ProfileManager.getLastUsedRegularProfile())
                            .resetPasswordManagerHandlerForTest();
                });
    }

    SettingsActivity startPasswordSettingsFromMainSettings(
            SettingsActivityTestRule<PasswordSettings> testRule) {
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putInt(
                BravePasswordManagerHelper.MANAGE_PASSWORDS_REFERRER,
                ManagePasswordsReferrer.CHROME_SETTINGS);
        mActivityToCleanUp = testRule.startSettingsActivity(fragmentArgs);
        return mActivityToCleanUp;
    }

    SettingsActivity startPasswordSettingsDirectly(
            SettingsActivityTestRule<PasswordSettings> testRule) {
        Bundle fragmentArgs = new Bundle();
        // The passwords accessory sheet is one of the places that can launch password settings
        // directly (without passing through main settings).
        fragmentArgs.putInt(
                BravePasswordManagerHelper.MANAGE_PASSWORDS_REFERRER,
                ManagePasswordsReferrer.PASSWORDS_ACCESSORY_SHEET);
        mActivityToCleanUp = testRule.startSettingsActivity(fragmentArgs);
        return mActivityToCleanUp;
    }
}
