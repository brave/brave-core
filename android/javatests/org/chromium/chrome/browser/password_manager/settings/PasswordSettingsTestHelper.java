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
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.browser.password_manager.BravePasswordManagerHelper;
import org.chromium.chrome.browser.password_manager.FakePasswordManagerHandler;
import org.chromium.chrome.browser.password_manager.ManagePasswordsReferrer;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.settings.SettingsActivity;
import org.chromium.chrome.browser.settings.SettingsActivityTestRule;

import java.util.ArrayList;
import java.util.Arrays;

/** Helper functions used in various password settings test suites. */
class PasswordSettingsTestHelper {
    static final SavedPasswordEntry ZEUS_ON_EARTH =
            new SavedPasswordEntry("http://www.phoenicia.gr", "Zeus", "Europa");
    static final SavedPasswordEntry ARES_AT_OLYMP =
            new SavedPasswordEntry("https://1-of-12.olymp.gr", "Ares", "God-o'w@r");
    static final SavedPasswordEntry PHOBOS_AT_OLYMP =
            new SavedPasswordEntry("https://visitor.olymp.gr", "Phobos-son-of-ares", "G0d0fF34r");
    static final SavedPasswordEntry DEIMOS_AT_OLYMP =
            new SavedPasswordEntry("https://visitor.olymp.gr", "Deimops-Ares-son", "G0d0fT3rr0r");
    static final SavedPasswordEntry HADES_AT_UNDERWORLD =
            new SavedPasswordEntry("https://underworld.gr", "", "C3rb3rus");
    static final SavedPasswordEntry[] GREEK_GODS = {
        ZEUS_ON_EARTH, ARES_AT_OLYMP, PHOBOS_AT_OLYMP, DEIMOS_AT_OLYMP, HADES_AT_UNDERWORLD,
    };

    private SettingsActivity mActivityToCleanUp;

    /**
     * Used to provide fake lists of stored passwords. Tests which need it can use setPasswordSource
     */
    private FakePasswordManagerHandler mHandler;

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

    /**
     * Helper to set up a fake source of displayed passwords.
     *
     * @param entry An entry to be added to saved passwords. Can be null.
     */
    void setPasswordSource(SavedPasswordEntry entry) {
        SavedPasswordEntry[] entries = {};
        if (entry != null) {
            entries = new SavedPasswordEntry[] {entry};
        }
        setPasswordSourceWithMultipleEntries(entries);
    }

    /**
     * Helper to set up a fake source of displayed passwords with multiple initial passwords.
     *
     * @param initialEntries All entries to be added to saved passwords. Can not be null.
     */
    void setPasswordSourceWithMultipleEntries(SavedPasswordEntry[] initialEntries) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    if (!ChromeBrowserInitializer.getInstance().isFullBrowserInitialized()) {
                        ChromeBrowserInitializer.getInstance().handleSynchronousStartup();
                    }
                });

        PasswordManagerHandlerProvider handlerProvider =
                ThreadUtils.runOnUiThreadBlocking(
                        () ->
                                PasswordManagerHandlerProvider.getForProfile(
                                        ProfileManager.getLastUsedRegularProfile()));
        if (mHandler == null) {
            mHandler = new FakePasswordManagerHandler(handlerProvider);
        }
        ArrayList<SavedPasswordEntry> entries = new ArrayList<>(Arrays.asList(initialEntries));
        mHandler.setSavedPasswords(entries);
        ThreadUtils.runOnUiThreadBlocking(
                () -> handlerProvider.setPasswordManagerHandlerForTest(mHandler));
    }

    /**
     * Helper to set up a fake source of displayed passwords without passwords but with exceptions.
     *
     * @param exceptions All exceptions to be added to saved exceptions. Can not be null.
     */
    void setPasswordExceptions(String[] exceptions) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    if (!ChromeBrowserInitializer.getInstance().isFullBrowserInitialized()) {
                        ChromeBrowserInitializer.getInstance().handleSynchronousStartup();
                    }
                });

        PasswordManagerHandlerProvider handlerProvider =
                ThreadUtils.runOnUiThreadBlocking(
                        () ->
                                PasswordManagerHandlerProvider.getForProfile(
                                        ProfileManager.getLastUsedRegularProfile()));
        if (mHandler == null) {
            mHandler = new FakePasswordManagerHandler(handlerProvider);
        }
        mHandler.setSavedPasswordExceptions(new ArrayList<>(Arrays.asList(exceptions)));
        ThreadUtils.runOnUiThreadBlocking(
                () -> handlerProvider.setPasswordManagerHandlerForTest(mHandler));
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
