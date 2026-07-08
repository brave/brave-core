/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.app.Activity;
import android.os.Build.VERSION_CODES;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.ActivityState;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.device_reauth.ReauthenticatorBridge;
import org.chromium.chrome.browser.incognito.reauth.BraveBrowserLockCoordinator;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthSettingUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;

@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = VERSION_CODES.R)
public class BraveBrowserLockManagerTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;
    @Mock private ReauthenticatorBridge mReauthenticatorBridge;
    @Mock private BraveBrowserLockCoordinator mMockCoordinator;

    private Activity mActivity;

    private class TestManager extends BraveBrowserLockManager {
        @Override
        BraveBrowserLockCoordinator createCoordinator(
                Activity activity, IncognitoReauthManager incognitoReauthManager) {
            return mMockCoordinator;
        }
    }

    @Before
    public void setUp() {
        mActivity = Robolectric.buildActivity(Activity.class).create().get();
        ReauthenticatorBridge.setInstanceForTesting(mReauthenticatorBridge);
        IncognitoReauthManager.setIsIncognitoReauthFeatureAvailableForTesting(true);
        IncognitoReauthSettingUtils.setIsDeviceScreenLockEnabledForTesting(true);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
    }

    private BraveBrowserLockManager createManager() {
        BraveBrowserLockManager manager = new BraveBrowserLockManager();
        manager.onNativeInitialized(mProfile);
        return manager;
    }

    private TestManager createTestManager() {
        TestManager manager = new TestManager();
        manager.onNativeInitialized(mProfile);
        return manager;
    }

    // --- isBrowserLockEnabled ---

    @Test
    public void isBrowserLockEnabled_allConditionsMet_returnsTrue() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        assertTrue(BraveBrowserLockManager.isBrowserLockEnabled());
    }

    @Test
    public void isBrowserLockEnabled_featureUnavailable_returnsFalse() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        IncognitoReauthManager.setIsIncognitoReauthFeatureAvailableForTesting(false);
        assertFalse(BraveBrowserLockManager.isBrowserLockEnabled());
    }

    @Test
    public void isBrowserLockEnabled_deviceLockDisabled_returnsFalse() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        IncognitoReauthSettingUtils.setIsDeviceScreenLockEnabledForTesting(false);
        assertFalse(BraveBrowserLockManager.isBrowserLockEnabled());
    }

    @Test
    public void isBrowserLockEnabled_prefDisabled_returnsFalse() {
        assertFalse(BraveBrowserLockManager.isBrowserLockEnabled());
    }

    // --- Lock arming ---

    @Test
    public void appBackgrounded_lockDisabled_doesNotArmLock() {
        BraveBrowserLockManager manager = createManager();
        manager.setLockArmedForTesting(BraveBrowserLockManager.isBrowserLockEnabled());
        assertFalse(manager.isLockArmedForTesting());
    }

    // --- First-launch arming ---

    @Test
    public void firstLaunch_lockEnabled_armsAndShowsLock() {
        mActivity = Robolectric.buildActivity(Activity.class).create().start().get();
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);

        TestManager manager = new TestManager();
        assertFalse(manager.isLockArmedForTesting());

        manager.onNativeInitialized(mProfile);

        assertTrue(manager.isLockArmedForTesting());
        verify(mMockCoordinator).show();
    }

    @Test
    public void firstLaunch_lockDisabled_doesNotArmOrShowLock() {
        mActivity = Robolectric.buildActivity(Activity.class).create().start().get();

        TestManager manager = new TestManager();
        manager.onNativeInitialized(mProfile);

        assertFalse(manager.isLockArmedForTesting());
        verify(mMockCoordinator, never()).show();
    }

    @Test
    public void secondNativeInit_alreadyAuthenticated_doesNotRearmLock() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);

        TestManager manager = new TestManager();
        manager.onNativeInitialized(mProfile);
        assertTrue(manager.isLockArmedForTesting());

        manager.getReauthCallbackForTesting().onIncognitoReauthSuccess();
        assertFalse(manager.isLockArmedForTesting());

        manager.onNativeInitialized(mProfile);
        assertFalse(manager.isLockArmedForTesting());
        verify(mMockCoordinator, never()).show();
    }

    @Test
    public void secondNativeInit_lockArmedByPriorBackground_catchUpShowsLock() {
        mActivity = Robolectric.buildActivity(Activity.class).create().start().get();
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);

        TestManager manager = new TestManager();
        manager.setNativeInitializedOnceForTesting(true);
        manager.setLockArmedForTesting(true);

        manager.onNativeInitialized(mProfile);

        verify(mMockCoordinator).show();
    }

    // --- onActivityStateChange ---

    @Test
    public void onActivityStateChange_started_lockArmed_showsLock() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);

        TestManager manager = createTestManager();
        manager.setLockArmedForTesting(true);

        manager.onActivityStateChange(mActivity, ActivityState.STARTED);
        verify(mMockCoordinator).show();
    }

    @Test
    public void onActivityStateChange_started_lockNotArmed_doesNotShowLock() {
        // Pref is deliberately left disabled so onNativeInitialized does not arm the lock.
        TestManager manager = createTestManager();
        assertFalse(manager.isLockArmedForTesting());

        manager.onActivityStateChange(mActivity, ActivityState.STARTED);
        verify(mMockCoordinator, never()).show();
    }

    // --- Reauth callbacks ---

    @Test
    public void reauthSuccess_clearsLockArmed() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        BraveBrowserLockManager manager = createManager();
        manager.setLockArmedForTesting(true);

        manager.getReauthCallbackForTesting().onIncognitoReauthSuccess();
        assertFalse(manager.isLockArmedForTesting());
    }

    @Test
    public void reauthFailure_keepsLockArmed() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        BraveBrowserLockManager manager = createManager();
        manager.setLockArmedForTesting(true);

        manager.getReauthCallbackForTesting().onIncognitoReauthFailure();
        assertTrue(manager.isLockArmedForTesting());
    }

    @Test
    public void reauthNotPossible_clearsLockArmed() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        BraveBrowserLockManager manager = createManager();
        manager.setLockArmedForTesting(true);

        manager.getReauthCallbackForTesting().onIncognitoReauthNotPossible();
        assertFalse(manager.isLockArmedForTesting());
    }
}
