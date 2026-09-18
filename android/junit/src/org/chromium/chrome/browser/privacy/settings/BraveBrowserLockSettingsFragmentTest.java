/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.os.Bundle;
import android.widget.RadioButton;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentFactory;
import androidx.fragment.app.testing.FragmentScenario;
import androidx.test.filters.SmallTest;

import com.google.android.material.materialswitch.MaterialSwitch;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Callback;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.device_reauth.BiometricStatus;
import org.chromium.chrome.browser.device_reauth.ReauthenticatorBridge;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthSettingUtils;
import org.chromium.chrome.browser.preferences.Pref;
import org.chromium.chrome.browser.privacy.BraveBrowserLockManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.ProfileDependentSetting;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

/**
 * Tests for {@link BraveBrowserLockSettingsFragment}.
 *
 * <p>Regression coverage for a bug where the "Private tabs" toggle wrote only a Brave-only pref
 * that nothing read for enforcement, silently making private-tab locking non-functional. The toggle
 * must read/write Chromium's own {@link Pref#INCOGNITO_REAUTHENTICATION_FOR_ANDROID} — the pref
 * {@link org.chromium.chrome.browser.incognito.reauth.IncognitoReauthControllerImpl} actually gates
 * on.
 *
 * <p>Also covers the "Screen capture" 3-way radio choice: it must work independently of the two
 * lock toggles, require authentication to change (except re-selecting the current option, which
 * must be a no-op), and revert on a failed/impossible reauth.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveBrowserLockSettingsFragmentTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;
    @Mock private PrefService mPrefService;
    @Mock private ReauthenticatorBridge mReauthenticatorBridge;

    // Mockito doesn't link a stubbed setter to a getter automatically — back the mock with real
    // state so a write is reflected by the next read, matching how the real PrefService behaves.
    private final boolean[] mIncognitoReauthPref = {false};

    @Before
    public void setUp() {
        UserPrefs.setPrefServiceForTesting(mPrefService);
        doAnswer(invocation -> mIncognitoReauthPref[0])
                .when(mPrefService)
                .getBoolean(Pref.INCOGNITO_REAUTHENTICATION_FOR_ANDROID);
        doAnswer(
                        invocation -> {
                            mIncognitoReauthPref[0] = invocation.getArgument(1);
                            return null;
                        })
                .when(mPrefService)
                .setBoolean(eq(Pref.INCOGNITO_REAUTHENTICATION_FOR_ANDROID), anyBoolean());

        IncognitoReauthManager.setIsIncognitoReauthFeatureAvailableForTesting(true);
        IncognitoReauthSettingUtils.setIsDeviceScreenLockEnabledForTesting(true);

        ReauthenticatorBridge.setInstanceForTesting(mReauthenticatorBridge);
        doReturn(BiometricStatus.BIOMETRICS_AVAILABLE)
                .when(mReauthenticatorBridge)
                .getBiometricAvailabilityStatus();
    }

    private void stubReauthResult(boolean success) {
        doAnswer(
                        invocation -> {
                            Callback<Boolean> callback = invocation.getArgument(0);
                            callback.onResult(success);
                            return null;
                        })
                .when(mReauthenticatorBridge)
                .reauthenticate(any());
    }

    @Test
    @SmallTest
    public void privateTabsToggle_onReauthSuccess_writesRealIncognitoReauthPref() {
        stubReauthResult(/* success= */ true);

        buildFragmentScenario()
                .onFragment(
                        fragment -> {
                            MaterialSwitch switchPrivateTabs =
                                    fragment.requireView().findViewById(R.id.switch_private_tabs);
                            switchPrivateTabs.performClick();

                            verify(mPrefService)
                                    .setBoolean(
                                            Pref.INCOGNITO_REAUTHENTICATION_FOR_ANDROID,
                                            /* value= */ true);
                            assertTrue(switchPrivateTabs.isChecked());
                        });
    }

    @Test
    @SmallTest
    public void privateTabsToggle_onReauthFailure_revertsWithoutWritingPref() {
        stubReauthResult(/* success= */ false);

        buildFragmentScenario()
                .onFragment(
                        fragment -> {
                            MaterialSwitch switchPrivateTabs =
                                    fragment.requireView().findViewById(R.id.switch_private_tabs);
                            switchPrivateTabs.performClick();

                            assertFalse(switchPrivateTabs.isChecked());
                            verify(mPrefService, never())
                                    .setBoolean(
                                            eq(Pref.INCOGNITO_REAUTHENTICATION_FOR_ANDROID),
                                            anyBoolean());
                        });
    }

    @Test
    @SmallTest
    public void screenshotModeRadio_defaultsToPrivateTabsOnly() {
        // Fresh Robolectric environment: no FeatureOverrides set, native uninitialized, so
        // ChromeFeatureList.sIncognitoScreenshot falls back to its own disabled-by-default value
        // — migrating to PRIVATE_TABS_ONLY, matching what every fresh install already had.
        buildFragmentScenario()
                .onFragment(
                        fragment -> {
                            RadioButton radio =
                                    fragment.requireView()
                                            .findViewById(R.id.radio_screenshot_private_tabs_only);
                            assertTrue(radio.isChecked());
                        });
    }

    @Test
    @SmallTest
    public void screenshotModeRadio_tappingAlreadySelected_doesNothing() {
        // Regression coverage for the explicit requirement: re-selecting the current option must
        // not go through authentication at all. The radio indicator itself is non-clickable (it
        // sits at the end of the row, matching the design), so the row's own click listener must
        // guard against re-selecting the current mode.
        buildFragmentScenario()
                .onFragment(
                        fragment -> {
                            fragment.requireView()
                                    .findViewById(R.id.row_screenshot_private_tabs_only)
                                    .performClick();

                            verify(mReauthenticatorBridge, never()).reauthenticate(any());
                            assertEquals(
                                    BravePreferenceKeys
                                            .BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_PRIVATE_TABS_ONLY,
                                    BraveBrowserLockManager.getScreenshotMode());
                        });
    }

    @Test
    @SmallTest
    public void screenshotModeRadio_selectEverything_onReauthSuccess_forcesSecureWindow() {
        stubReauthResult(/* success= */ true);

        buildFragmentScenario()
                .onFragment(
                        fragment -> {
                            fragment.requireView()
                                    .findViewById(R.id.row_screenshot_everything)
                                    .performClick();

                            assertEquals(
                                    BravePreferenceKeys
                                            .BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_EVERYTHING,
                                    BraveBrowserLockManager.getScreenshotMode());
                            assertTrue(BraveBrowserLockManager.shouldForceSecureWindow());

                            RadioButton radio =
                                    fragment.requireView()
                                            .findViewById(R.id.radio_screenshot_everything);
                            assertTrue(radio.isChecked());
                        });
    }

    @Test
    @SmallTest
    public void screenshotModeRadio_worksWithNoLocksEnabled() {
        // The radio choice must not require "Entire application" or "Private tabs" to already be
        // enabled — it is independently useful and already gated on its own authentication.
        stubReauthResult(/* success= */ true);

        buildFragmentScenario()
                .onFragment(
                        fragment -> {
                            assertFalse(BraveBrowserLockManager.isBrowserLockEnabled());

                            fragment.requireView()
                                    .findViewById(R.id.row_screenshot_allow)
                                    .performClick();

                            assertEquals(
                                    BravePreferenceKeys.BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_ALLOW,
                                    BraveBrowserLockManager.getScreenshotMode());
                        });
    }

    @Test
    @SmallTest
    public void screenshotModeRadio_onReauthFailure_leavesSelectionUnchanged() {
        stubReauthResult(/* success= */ false);

        buildFragmentScenario()
                .onFragment(
                        fragment -> {
                            fragment.requireView()
                                    .findViewById(R.id.row_screenshot_allow)
                                    .performClick();

                            RadioButton radio =
                                    fragment.requireView()
                                            .findViewById(R.id.radio_screenshot_private_tabs_only);
                            assertTrue(radio.isChecked());
                            assertEquals(
                                    BravePreferenceKeys
                                            .BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_PRIVATE_TABS_ONLY,
                                    BraveBrowserLockManager.getScreenshotMode());
                        });
    }

    @Test
    @SmallTest
    public void screenshotModeRadio_onReauthNotPossible_leavesSelectionUnchanged() {
        doReturn(BiometricStatus.UNAVAILABLE)
                .when(mReauthenticatorBridge)
                .getBiometricAvailabilityStatus();

        buildFragmentScenario()
                .onFragment(
                        fragment -> {
                            fragment.requireView()
                                    .findViewById(R.id.row_screenshot_everything)
                                    .performClick();

                            RadioButton radio =
                                    fragment.requireView()
                                            .findViewById(R.id.radio_screenshot_private_tabs_only);
                            assertTrue(radio.isChecked());
                            assertFalse(BraveBrowserLockManager.shouldForceSecureWindow());
                        });
    }

    @Test
    @SmallTest
    public void screenshotModeRadio_whileReauthPending_doesNotChangeSelectionOrPrefYet() {
        // Deliberately do not stub a reauth result, so the callback never fires and the flow
        // stays pending — verifying the selection and the underlying pref only change on
        // confirmed success, never optimistically on tap.
        buildFragmentScenario()
                .onFragment(
                        fragment -> {
                            fragment.requireView()
                                    .findViewById(R.id.row_screenshot_everything)
                                    .performClick();

                            RadioButton radio =
                                    fragment.requireView()
                                            .findViewById(R.id.radio_screenshot_private_tabs_only);
                            assertTrue(radio.isChecked());
                            assertEquals(
                                    BravePreferenceKeys
                                            .BRAVE_BROWSER_LOCK_SCREENSHOT_MODE_PRIVATE_TABS_ONLY,
                                    BraveBrowserLockManager.getScreenshotMode());
                            assertFalse(BraveBrowserLockManager.shouldForceSecureWindow());
                        });
    }

    private FragmentScenario<BraveBrowserLockSettingsFragment> buildFragmentScenario() {
        return FragmentScenario.launchInContainer(
                BraveBrowserLockSettingsFragment.class,
                Bundle.EMPTY,
                R.style.Theme_Chromium_Settings,
                new FragmentFactory() {
                    @Override
                    public Fragment instantiate(
                            @NonNull ClassLoader classLoader, @NonNull String className) {
                        Fragment fragment = super.instantiate(classLoader, className);
                        if (fragment instanceof ProfileDependentSetting) {
                            ((ProfileDependentSetting) fragment).setProfile(mProfile);
                        }
                        return fragment;
                    }
                });
    }
}
