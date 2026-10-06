/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.app.Activity;
import android.os.Build.VERSION_CODES;
import android.view.WindowManager;

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
import org.chromium.base.FeatureOverrides;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.device_reauth.ReauthenticatorBridge;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.incognito.reauth.BraveBrowserLockCoordinator;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthSettingUtils;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;

import java.util.HashMap;
import java.util.Map;

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

    private final Map<Activity, BraveBrowserLockCoordinator> mCoordinatorsByActivity =
            new HashMap<>();

    private class MultiInstanceTestManager extends BraveBrowserLockManager {
        @Override
        BraveBrowserLockCoordinator createCoordinator(
                Activity activity, IncognitoReauthManager incognitoReauthManager) {
            BraveBrowserLockCoordinator coordinator = mock(BraveBrowserLockCoordinator.class);
            mCoordinatorsByActivity.put(activity, coordinator);
            return coordinator;
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
    public void preNativeOverlay_lockArmed_overlayShownOnActivityStart() {
        mActivity = Robolectric.buildActivity(Activity.class).create().start().get();

        TestManager manager = new TestManager(); // mProfile == null, no onNativeInitialized
        manager.setLockArmedForTesting(true);

        manager.onActivityStateChange(mActivity, ActivityState.STARTED);

        assertTrue(manager.isPreNativeOverlayShownForTesting(mActivity));
    }

    @Test
    public void preNativeOverlay_lockNotArmed_noOverlay() {
        mActivity = Robolectric.buildActivity(Activity.class).create().start().get();

        TestManager manager = new TestManager();
        // mLockArmed stays false

        manager.onActivityStateChange(mActivity, ActivityState.STARTED);

        assertFalse(manager.isPreNativeOverlayShownForTesting(mActivity));
    }

    @Test
    public void preNativeOverlay_removedOnNativeInitialized() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        mActivity = Robolectric.buildActivity(Activity.class).create().start().get();

        TestManager manager = new TestManager();
        manager.setLockArmedForTesting(true);
        manager.onActivityStateChange(mActivity, ActivityState.STARTED);
        assertTrue(manager.isPreNativeOverlayShownForTesting(mActivity));

        manager.onNativeInitialized(mProfile);

        assertFalse(manager.isPreNativeOverlayShownForTesting(mActivity));
    }

    @Test
    public void onNativeInitialized_featureUnavailable_clearsPhase1Arm() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        IncognitoReauthManager.setIsIncognitoReauthFeatureAvailableForTesting(false);

        BraveBrowserLockManager manager = new BraveBrowserLockManager();
        manager.setLockArmedForTesting(true); // simulate Phase 1 pref-only arming

        manager.onNativeInitialized(mProfile);

        assertFalse(manager.isLockArmedForTesting());
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

    @Test
    public void secondActivityStarted_whileFirstLocked_alsoGetsLocked() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        Activity secondActivity = Robolectric.buildActivity(Activity.class).create().get();

        MultiInstanceTestManager manager = new MultiInstanceTestManager();
        manager.onNativeInitialized(mProfile);
        manager.setLockArmedForTesting(true);

        manager.onActivityStateChange(mActivity, ActivityState.STARTED);
        manager.onActivityStateChange(secondActivity, ActivityState.STARTED);

        assertTrue(manager.isLockShownForTesting(mActivity));
        assertTrue(manager.isLockShownForTesting(secondActivity));
        assertEquals(2, manager.getActiveLockCountForTesting());
        verify(mCoordinatorsByActivity.get(mActivity)).show();
        verify(mCoordinatorsByActivity.get(secondActivity)).show();
    }

    @Test
    public void secondActivityStarted_whileFirstReauthPending_doesNotStartConcurrentReauth() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        Activity secondActivity = Robolectric.buildActivity(Activity.class).create().get();

        MultiInstanceTestManager manager = new MultiInstanceTestManager();
        manager.onNativeInitialized(mProfile);
        manager.setLockArmedForTesting(true);

        manager.onActivityStateChange(mActivity, ActivityState.STARTED);
        assertTrue(manager.isReauthInFlightForTesting());

        // The unstubbed mock ReauthenticatorBridge never calls back, so the first attempt stays
        // pending. The second activity must still get its overlay, but must not trigger a second
        // concurrent biometric prompt.
        manager.onActivityStateChange(secondActivity, ActivityState.STARTED);

        assertTrue(manager.isLockShownForTesting(secondActivity));
        assertTrue(manager.isReauthInFlightForTesting());
        assertEquals(2, manager.getActiveLockCountForTesting());
    }

    @Test
    public void reauthFailure_startsQueuedSecondActivityReauth() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        Activity secondActivity = Robolectric.buildActivity(Activity.class).create().get();

        MultiInstanceTestManager manager = new MultiInstanceTestManager();
        manager.onNativeInitialized(mProfile);
        manager.setLockArmedForTesting(true);

        manager.onActivityStateChange(mActivity, ActivityState.STARTED);
        manager.onActivityStateChange(secondActivity, ActivityState.STARTED);
        assertTrue(manager.isReauthInFlightForTesting());

        manager.getReauthCallbackForTesting().onIncognitoReauthFailure();

        // Still armed (failure doesn't unlock), but a new attempt should now be in-flight for
        // whichever activity hadn't been tried yet.
        assertTrue(manager.isLockArmedForTesting());
        assertTrue(manager.isReauthInFlightForTesting());
        assertEquals(2, manager.getActiveLockCountForTesting());
    }

    @Test
    public void reauthSuccess_hidesAllActivityLocks() {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);
        Activity secondActivity = Robolectric.buildActivity(Activity.class).create().get();

        MultiInstanceTestManager manager = new MultiInstanceTestManager();
        manager.onNativeInitialized(mProfile);
        manager.setLockArmedForTesting(true);

        manager.onActivityStateChange(mActivity, ActivityState.STARTED);
        manager.onActivityStateChange(secondActivity, ActivityState.STARTED);

        manager.getReauthCallbackForTesting().onIncognitoReauthSuccess();

        assertFalse(manager.isLockArmedForTesting());
        assertFalse(manager.isLockShownForTesting(mActivity));
        assertFalse(manager.isLockShownForTesting(secondActivity));
        assertEquals(0, manager.getActiveLockCountForTesting());
        verify(mCoordinatorsByActivity.get(mActivity)).hide(anyInt());
        verify(mCoordinatorsByActivity.get(secondActivity)).hide(anyInt());
    }

    // --- Pref change listener ---

    @Test
    public void prefDisabled_whileLockShown_hidesCoordinator() {
        // Regression coverage: disabling the lock pref (e.g. from a different multi-instance
        // window) must dismiss an already-showing lock, not just pre-native placeholder overlays.
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, true);

        TestManager manager = createTestManager();
        manager.setLockArmedForTesting(true);
        manager.onActivityStateChange(mActivity, ActivityState.STARTED);
        assertTrue(manager.isLockShownForTesting(mActivity));
        assertTrue(manager.isReauthInFlightForTesting());

        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
        manager.getPrefChangeListenerForTesting()
                .onSharedPreferenceChanged(
                        /* sharedPreferences= */ null, BravePreferenceKeys.BRAVE_BROWSER_LOCK);

        assertFalse(manager.isLockShownForTesting(mActivity));
        assertFalse(manager.isLockArmedForTesting());
        assertFalse(manager.isReauthInFlightForTesting());
        verify(mMockCoordinator).hide(anyInt());
    }

    // --- applySecureFlagToAllActivities ---

    @Test
    public void applySecureFlag_doesNotClearFlagItDidNotSet() {
        // Regression coverage: an upstream per-activity incognito-aware controller may have set
        // FLAG_SECURE on its own (e.g. a currently-showing incognito tab under
        // PRIVATE_TABS_ONLY) — Brave's own sweep must leave it alone since it never set it.
        BraveBrowserLockManager manager = createManager();
        mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_SECURE);
        assertFalse(manager.isForcedSecureForTesting(mActivity));

        manager.getPrefChangeListenerForTesting()
                .onSharedPreferenceChanged(
                        /* sharedPreferences= */ null, BravePreferenceKeys.BRAVE_BROWSER_LOCK);

        assertTrue(
                (mActivity.getWindow().getAttributes().flags
                                & WindowManager.LayoutParams.FLAG_SECURE)
                        != 0);
    }

    @Test
    public void applySecureFlag_clearsFlagItSetItself() {
        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING);
        BraveBrowserLockManager manager = createManager();
        assertTrue(manager.isForcedSecureForTesting(mActivity));

        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_PRIVATE_TABS_ONLY);
        manager.getPrefChangeListenerForTesting()
                .onSharedPreferenceChanged(
                        /* sharedPreferences= */ null,
                        BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE);

        assertFalse(manager.isForcedSecureForTesting(mActivity));
        assertEquals(
                0,
                mActivity.getWindow().getAttributes().flags
                        & WindowManager.LayoutParams.FLAG_SECURE);
    }

    // --- shouldSecureForIncognitoVisibility ---

    @Test
    public void shouldSecureForIncognitoVisibility_everything_alwaysTrue() {
        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING);

        assertTrue(BraveBrowserLockManager.shouldSecureForIncognitoVisibility(false));
        assertTrue(BraveBrowserLockManager.shouldSecureForIncognitoVisibility(true));
    }

    @Test
    public void shouldSecureForIncognitoVisibility_allow_alwaysFalse() {
        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_ALLOW);

        assertFalse(BraveBrowserLockManager.shouldSecureForIncognitoVisibility(false));
        assertFalse(BraveBrowserLockManager.shouldSecureForIncognitoVisibility(true));
    }

    @Test
    public void shouldSecureForIncognitoVisibility_privateTabsOnly_matchesVisibility() {
        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_PRIVATE_TABS_ONLY);

        assertFalse(BraveBrowserLockManager.shouldSecureForIncognitoVisibility(false));
        assertTrue(BraveBrowserLockManager.shouldSecureForIncognitoVisibility(true));
    }

    // --- ScreenshotModeObserver ---

    @Test
    public void setScreenshotMode_notifiesRegisteredObserver() {
        // Regression coverage: a live per-activity incognito screenshot controller must recompute
        // immediately on a mode change, rather than waiting for its own unrelated trigger.
        BraveBrowserLockManager manager = createManager();
        BraveBrowserLockManager.setInstanceForTesting(manager);
        BraveBrowserLockManager.ScreenshotModeObserver observer =
                mock(BraveBrowserLockManager.ScreenshotModeObserver.class);
        BraveBrowserLockManager.addScreenshotModeObserver(mActivity, observer);

        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING);

        verify(observer).onScreenshotModeChanged();
    }

    @Test
    public void setScreenshotMode_withNoRegisteredObserver_doesNotThrow() {
        BraveBrowserLockManager manager = createManager();
        BraveBrowserLockManager.setInstanceForTesting(manager);

        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_ALLOW);
    }

    @Test
    public void screenshotModeObserver_unregisteredOnActivityDestroyed() {
        BraveBrowserLockManager manager = createManager();
        BraveBrowserLockManager.setInstanceForTesting(manager);
        BraveBrowserLockManager.ScreenshotModeObserver observer =
                mock(BraveBrowserLockManager.ScreenshotModeObserver.class);
        BraveBrowserLockManager.addScreenshotModeObserver(mActivity, observer);

        manager.getAppLifecycleCallbacksForTesting().onActivityDestroyed(mActivity);
        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING);

        verify(observer, never()).onScreenshotModeChanged();
    }

    // --- getScreenshotMode / shouldForceSecureWindow ---

    @Test
    public void getScreenshotMode_prefUnset_incognitoScreenshotFeatureEnabled_migratesToAllow() {
        FeatureOverrides.enable(ChromeFeatureList.INCOGNITO_SCREENSHOT);

        assertEquals(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_ALLOW,
                BraveBrowserLockManager.getScreenshotMode());
    }

    @Test
    public void
            getScreenshotMode_prefUnset_incognitoScreenshotFeatureDisabled_migratesToPrivateTabsOnly() {
        FeatureOverrides.disable(ChromeFeatureList.INCOGNITO_SCREENSHOT);

        assertEquals(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_PRIVATE_TABS_ONLY,
                BraveBrowserLockManager.getScreenshotMode());
    }

    @Test
    public void getScreenshotMode_neverMigratesToEverything() {
        // EVERYTHING has no pre-existing equivalent — it must never be auto-selected regardless
        // of the old feature's state.
        FeatureOverrides.enable(ChromeFeatureList.INCOGNITO_SCREENSHOT);
        assertFalse(
                BraveBrowserLockManager.getScreenshotMode()
                        == BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING);

        FeatureOverrides.disable(ChromeFeatureList.INCOGNITO_SCREENSHOT);
        assertFalse(
                BraveBrowserLockManager.getScreenshotMode()
                        == BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING);
    }

    @Test
    public void getScreenshotMode_prefAlreadySet_ignoresFeatureState() {
        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING);
        // If the persisted value were ignored in favor of re-deriving from the feature, this would
        // come back as ALLOW instead.
        FeatureOverrides.enable(ChromeFeatureList.INCOGNITO_SCREENSHOT);

        assertEquals(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING,
                BraveBrowserLockManager.getScreenshotMode());
    }

    @Test
    public void shouldForceSecureWindow_trueOnlyForEverythingMode() {
        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_ALLOW);
        assertFalse(BraveBrowserLockManager.shouldForceSecureWindow());

        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_PRIVATE_TABS_ONLY);
        assertFalse(BraveBrowserLockManager.shouldForceSecureWindow());

        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING);
        assertTrue(BraveBrowserLockManager.shouldForceSecureWindow());
    }

    @Test
    public void shouldForceSecureWindow_independentOfBrowserLockState() {
        // Screenshot protection must work even with no browser lock enabled at all — changing it
        // already requires authentication, so gating it behind a separate lock is unnecessary.
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_BROWSER_LOCK, false);
        BraveBrowserLockManager.setScreenshotMode(
                BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING);

        assertFalse(BraveBrowserLockManager.isBrowserLockEnabled());
        assertTrue(BraveBrowserLockManager.shouldForceSecureWindow());
    }
}
