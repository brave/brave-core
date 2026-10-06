/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.incognito.reauth;

import static org.mockito.ArgumentMatchers.notNull;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.Callback;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.device_reauth.BiometricStatus;
import org.chromium.chrome.browser.device_reauth.ReauthenticatorBridge;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager.IncognitoReauthCallback;

/**
 * Tests for {@link BraveIncognitoReauthManager}.
 *
 * <p>Regression coverage for a bug where the lock overlay's "Unlock" button called {@link
 * IncognitoReauthManager#startReauthenticationFlow} directly with no guard, so a second tap while
 * the first biometric prompt was still pending could invoke {@link
 * ReauthenticatorBridge#reauthenticate} concurrently on the same instance.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveIncognitoReauthManagerTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private ReauthenticatorBridge mReauthenticatorBridge;
    @Mock private IncognitoReauthCallback mFirstCallback;
    @Mock private IncognitoReauthCallback mSecondCallback;

    private BraveIncognitoReauthManager mManager;

    @Before
    public void setUp() {
        IncognitoReauthManager.setIsIncognitoReauthFeatureAvailableForTesting(true);
        when(mReauthenticatorBridge.getBiometricAvailabilityStatus())
                .thenReturn(BiometricStatus.BIOMETRICS_AVAILABLE);
        mManager = new BraveIncognitoReauthManager(mReauthenticatorBridge);
    }

    @Test
    public void startReauthenticationFlow_whileFlowInProgress_ignoresSecondCall() {
        // Never invoke the ReauthenticatorBridge's callback, simulating a still-pending prompt.
        mManager.startReauthenticationFlow(mFirstCallback);

        mManager.startReauthenticationFlow(mSecondCallback);

        verify(mReauthenticatorBridge, times(1)).reauthenticate(notNull());
        verify(mSecondCallback, never()).onIncognitoReauthNotPossible();
        verify(mSecondCallback, never()).onIncognitoReauthSuccess();
        verify(mSecondCallback, never()).onIncognitoReauthFailure();
        verifyNoMoreInteractions(mSecondCallback);
    }

    @Test
    public void startReauthenticationFlow_afterSuccess_allowsNextCall() {
        doAnswer(
                        invocation -> {
                            Callback<Boolean> callback = invocation.getArgument(0);
                            callback.onResult(true);
                            return null;
                        })
                .when(mReauthenticatorBridge)
                .reauthenticate(notNull());

        mManager.startReauthenticationFlow(mFirstCallback);
        verify(mFirstCallback).onIncognitoReauthSuccess();

        mManager.startReauthenticationFlow(mSecondCallback);
        verify(mSecondCallback).onIncognitoReauthSuccess();

        verify(mReauthenticatorBridge, times(2)).reauthenticate(notNull());
    }

    @Test
    public void startReauthenticationFlow_afterFailure_allowsNextCall() {
        doAnswer(
                        invocation -> {
                            Callback<Boolean> callback = invocation.getArgument(0);
                            callback.onResult(false);
                            return null;
                        })
                .when(mReauthenticatorBridge)
                .reauthenticate(notNull());

        mManager.startReauthenticationFlow(mFirstCallback);
        verify(mFirstCallback).onIncognitoReauthFailure();

        mManager.startReauthenticationFlow(mSecondCallback);
        verify(mSecondCallback).onIncognitoReauthFailure();

        verify(mReauthenticatorBridge, times(2)).reauthenticate(notNull());
    }

    @Test
    public void startReauthenticationFlow_afterNotPossible_allowsNextCall() {
        when(mReauthenticatorBridge.getBiometricAvailabilityStatus())
                .thenReturn(BiometricStatus.UNAVAILABLE);

        mManager.startReauthenticationFlow(mFirstCallback);
        verify(mFirstCallback).onIncognitoReauthNotPossible();

        mManager.startReauthenticationFlow(mSecondCallback);
        verify(mSecondCallback).onIncognitoReauthNotPossible();
    }
}
