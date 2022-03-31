/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.onboarding;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.onboarding.BraveTalkOptInPopupListener;

@JNINamespace("brave_ads")
public class BraveAdsHostAndroid implements BraveTalkOptInPopupListener {
    private long mNativeBraveAdsHostAndroid;

    public BraveAdsHostAndroid(long nativeBraveAdsHostAndroid) {
        ThreadUtils.assertOnUiThread();
        mNativeBraveAdsHostAndroid = nativeBraveAdsHostAndroid;
    }

    @Override
    public void notifyAdsEnableButtonPressed() {
        notifyAdsEnabled();
    }

    @Override
    public void notifyTalkOptInPopupClosed() {
        notifyPopupClosed();
    }

    @CalledByNative
    public static BraveAdsHostAndroid create(long nativeBraveAdsHostAndroid) {
        return new BraveAdsHostAndroid(nativeBraveAdsHostAndroid);
    }

    /**
     * Destroys this instance so no further calls can be executed.
     */
    @CalledByNative
    public void destroy() {
        mNativeBraveAdsHostAndroid = 0;

        BraveActivity braveActivity = BraveActivity.getBraveActivity();
        if (braveActivity != null) {
            braveActivity.closeBraveTalkOptInPopup();
        }
    }

    public void notifyAdsEnabled() {
        if (mNativeBraveAdsHostAndroid == 0) return;

        BraveAdsHostAndroidJni.get().notifyAdsEnabled(
                mNativeBraveAdsHostAndroid, BraveAdsHostAndroid.this);
    }

    public void notifyPopupClosed() {
        if (mNativeBraveAdsHostAndroid == 0) return;

        BraveAdsHostAndroidJni.get().notifyPopupClosed(
                mNativeBraveAdsHostAndroid, BraveAdsHostAndroid.this);
    }

    @CalledByNative
    public void openBraveTalkOptInPopup() {
        BraveActivity braveActivity = BraveActivity.getBraveActivity();
        if (braveActivity == null) {
            notifyPopupClosed();
            return;
        }

        braveActivity.openBraveTalkOptInPopup(this);
    }

    @NativeMethods
    interface Natives {
        void notifyAdsEnabled(long nativeBraveAdsHostAndroid, BraveAdsHostAndroid caller);
        void notifyPopupClosed(long nativeBraveAdsHostAndroid, BraveAdsHostAndroid caller);
    }
}
