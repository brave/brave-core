/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
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
import org.chromium.base.supplier.OneshotSupplierImpl;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.device_reauth.ReauthenticatorBridge;
import org.chromium.chrome.browser.incognito.reauth.BraveBrowserLockCoordinator;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthController;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthSettingUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;

@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = VERSION_CODES.R)
public class BraveBrowserLockControllerTest {
    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;
    @Mock private ReauthenticatorBridge mReauthenticatorBridge;
    @Mock private BraveBrowserLockCoordinator mMockCoordinator;
    @Mock private IncognitoReauthController mIncognitoReauthController;

    private Activity mActivity;
    private OneshotSupplierImpl<IncognitoReauthController> mIncognitoReauthControllerSupplier;

    /** Subclass that replaces coordinator creation with a mock, avoiding real UI construction. */
    private class TestController extends BraveBrowserLockController {
        TestController() {
            super(mActivity, mProfile, mActivity.getTaskId(), mIncognitoReauthControllerSupplier);
        }

        @Override
        BraveBrowserLockCoordinator createCoordinator(Activity activity) {
            return mMockCoordinator;
        }
    }

    @Before
    public void setUp() {
        mActivity = Robolectric.buildActivity(Activity.class).create().get();
        mIncognitoReauthControllerSupplier = new OneshotSupplierImpl<>();
        mIncognitoReauthControllerSupplier.set(mIncognitoReauthController);
        ReauthenticatorBridge.setInstanceForTesting(mReauthenticatorBridge);
        IncognitoReauthManager.setIsIncognitoReauthFeatureAvailableForTesting(true);
        IncognitoReauthSettingUtils.setIsDeviceScreenLockEnabledForTesting(true);
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
    }

    private BraveBrowserLockController createController() {
        return new BraveBrowserLockController(
                mActivity, mProfile, mActivity.getTaskId(), mIncognitoReauthControllerSupplier);
    }

    @Test
    public void isBrowserLockEnabled_allConditionsMet_returnsTrue() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        assertTrue(BraveBrowserLockController.isBrowserLockEnabled());
    }

    @Test
    public void isBrowserLockEnabled_featureUnavailable_returnsFalse() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        IncognitoReauthManager.setIsIncognitoReauthFeatureAvailableForTesting(false);
        assertFalse(BraveBrowserLockController.isBrowserLockEnabled());
    }

    @Test
    public void isBrowserLockEnabled_deviceLockDisabled_returnsFalse() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        IncognitoReauthSettingUtils.setIsDeviceScreenLockEnabledForTesting(false);
        assertFalse(BraveBrowserLockController.isBrowserLockEnabled());
    }

    @Test
    public void isBrowserLockEnabled_prefDisabled_returnsFalse() {
        assertFalse(BraveBrowserLockController.isBrowserLockEnabled());
    }

    // Constructor catch-up when activity already started.
    @Test
    public void constructor_alreadyStarted_lockDisabled_noCoordinatorCreated() {
        mActivity = Robolectric.buildActivity(Activity.class).create().start().get();
        BraveBrowserLockController controller = createController();
        assertFalse(controller.isLockPendingForTesting());
        controller.destroy();
    }

    @Test
    public void constructor_alreadyStarted_lockEnabled_showsLockImmediately() {
        mActivity = Robolectric.buildActivity(Activity.class).create().start().get();
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        TestController controller = new TestController();
        verify(mMockCoordinator).show();
        controller.destroy();
    }

    // mBrowserLockPending state machine
    @Test
    public void constructor_lockEnabled_armsLock() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        BraveBrowserLockController controller = createController();
        assertTrue(controller.isLockPendingForTesting());
        controller.destroy();
    }

    @Test
    public void constructor_lockDisabled_doesNotArmLock() {
        BraveBrowserLockController controller = createController();
        assertFalse(controller.isLockPendingForTesting());
        controller.destroy();
    }

    @Test
    public void onTaskVisibilityChanged_goesBackground_rearmsLock() {
        BraveBrowserLockController controller = createController();
        assertFalse(controller.isLockPendingForTesting());

        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        controller.onTaskVisibilityChanged(mActivity.getTaskId(), /* isVisible= */ false);
        assertTrue(controller.isLockPendingForTesting());
        controller.destroy();
    }

    @Test
    public void onTaskVisibilityChanged_goesBackground_lockDisabled_doesNotArmLock() {
        BraveBrowserLockController controller = createController();
        controller.onTaskVisibilityChanged(mActivity.getTaskId(), /* isVisible= */ false);
        assertFalse(controller.isLockPendingForTesting());
        controller.destroy();
    }

    @Test
    public void onTaskVisibilityChanged_differentTask_ignored() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        BraveBrowserLockController controller = createController();
        assertTrue(controller.isLockPendingForTesting());

        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
        controller.onTaskVisibilityChanged(mActivity.getTaskId() + 1, /* isVisible= */ false);
        assertTrue(controller.isLockPendingForTesting());
        controller.destroy();
    }

    // Reauth callback behaviour.
    @Test
    public void reauthSuccess_clearsLockPending() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        BraveBrowserLockController controller = createController();
        assertTrue(controller.isLockPendingForTesting());

        controller.getReauthCallbackForTesting().onIncognitoReauthSuccess();
        assertFalse(controller.isLockPendingForTesting());
        controller.destroy();
    }

    @Test
    public void reauthFailure_keepsLockPending() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        BraveBrowserLockController controller = createController();

        controller.getReauthCallbackForTesting().onIncognitoReauthFailure();
        assertTrue(controller.isLockPendingForTesting());
        controller.destroy();
    }

    @Test
    public void reauthNotPossible_keepsLockPending() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        BraveBrowserLockController controller = createController();

        controller.getReauthCallbackForTesting().onIncognitoReauthNotPossible();
        assertTrue(controller.isLockPendingForTesting());
        controller.destroy();
    }

    // onActivityStateChange drives lock display.

    @Test
    public void onActivityStateChange_started_lockPending_showsLock() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        TestController controller = new TestController();
        assertTrue(controller.isLockPendingForTesting());

        controller.onActivityStateChange(mActivity, ActivityState.STARTED);
        verify(mMockCoordinator).show();
        controller.destroy();
    }
}
