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
import android.view.View;

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

import org.chromium.base.Callback;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.device_reauth.BiometricStatus;
import org.chromium.chrome.browser.device_reauth.ReauthenticatorBridge;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthSettingUtils;
import org.chromium.chrome.browser.preferences.Pref;
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
    public void captureSection_hiddenUntilPrivateTabsLocked_thenShown() {
        stubReauthResult(/* success= */ true);

        buildFragmentScenario()
                .onFragment(
                        fragment -> {
                            View container =
                                    fragment.requireView()
                                            .findViewById(R.id.prevent_capture_container);
                            assertEquals(View.GONE, container.getVisibility());

                            MaterialSwitch switchPrivateTabs =
                                    fragment.requireView().findViewById(R.id.switch_private_tabs);
                            switchPrivateTabs.performClick();

                            assertEquals(View.VISIBLE, container.getVisibility());
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
