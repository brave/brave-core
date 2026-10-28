/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.incognito.reauth;

import android.app.Activity;

import androidx.annotation.VisibleForTesting;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.device_reauth.ReauthenticatorBridge;
import org.chromium.chrome.browser.profiles.Profile;

/**
 * Guards {@link #startReauthenticationFlow} against being invoked a second time while a flow it
 * started is still pending on the same instance.
 *
 * <p>{@link org.chromium.chrome.browser.privacy.BraveBrowserLockManager} already serializes
 * auto-triggered reauth across multiple locked activities via its own in-flight tracking, but the
 * lock overlay's "Unlock" button (wired by upstream's {@code IncognitoReauthMediator}) calls {@link
 * #startReauthenticationFlow} directly on the same manager instance with no such guard — a second
 * tap while the first biometric prompt is still showing would otherwise invoke {@link
 * org.chromium.chrome.browser.device_reauth.ReauthenticatorBridge#reauthenticate} concurrently.
 */
@NullMarked
public class BraveIncognitoReauthManager extends IncognitoReauthManager {
    private boolean mFlowInProgress;

    public BraveIncognitoReauthManager(Activity activity, Profile profile) {
        super(activity, profile);
    }

    @VisibleForTesting
    public BraveIncognitoReauthManager(ReauthenticatorBridge reauthenticatorBridge) {
        super(reauthenticatorBridge);
    }

    @Override
    public void startReauthenticationFlow(IncognitoReauthCallback incognitoReauthCallback) {
        if (mFlowInProgress) return;
        mFlowInProgress = true;
        super.startReauthenticationFlow(
                new IncognitoReauthCallback() {
                    @Override
                    public void onIncognitoReauthNotPossible() {
                        mFlowInProgress = false;
                        incognitoReauthCallback.onIncognitoReauthNotPossible();
                    }

                    @Override
                    public void onIncognitoReauthSuccess() {
                        mFlowInProgress = false;
                        incognitoReauthCallback.onIncognitoReauthSuccess();
                    }

                    @Override
                    public void onIncognitoReauthFailure() {
                        mFlowInProgress = false;
                        incognitoReauthCallback.onIncognitoReauthFailure();
                    }
                });
    }
}
