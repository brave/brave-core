/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.profiles.Profile;

@JNINamespace("chrome::android")
public class BravePrefServiceBridge {
    private BravePrefServiceBridge() {
    }

    private static BravePrefServiceBridge sInstance;

    public static BravePrefServiceBridge getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = new BravePrefServiceBridge();
        }
        return sInstance;
    }

    /**
     * @param whether HTTPSE should be enabled.
     */
    public void setHTTPSEEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setHTTPSEEnabled(enabled);
    }

    /**
     * @param whether AdBlock should be enabled.
     */
    public void setAdBlockEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setAdBlockEnabled(enabled);
    }

    /**
     * @param whether Fingerprinting Protection should be enabled.
     */
    public void setFingerprintingProtectionEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setFingerprintingProtectionEnabled(enabled);
    }

    public void setPlayYTVideoInBrowserEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setPlayYTVideoInBrowserEnabled(enabled);
    }

    public boolean getPlayYTVideoInBrowserEnabled() {
        return BravePrefServiceBridgeJni.get().getPlayYTVideoInBrowserEnabled();
    }

    public void setDesktopModeEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setDesktopModeEnabled(enabled);
    }

    public boolean getDesktopModeEnabled() {
        return BravePrefServiceBridgeJni.get().getDesktopModeEnabled();
    }

    public void setBackgroundVideoPlaybackEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setBackgroundVideoPlaybackEnabled(enabled);
    }

    public boolean getBackgroundVideoPlaybackEnabled() {
        return BravePrefServiceBridgeJni.get().getBackgroundVideoPlaybackEnabled();
    }

    public int getTrackersBlockedCount(Profile profile) {
        return BravePrefServiceBridgeJni.get().getTrackersBlockedCount(profile);
    }

    public int getAdsBlockedCount(Profile profile) {
        return BravePrefServiceBridgeJni.get().getAdsBlockedCount(profile);
    }

    public int getHttpsUpgradesCount(Profile profile) {
        return BravePrefServiceBridgeJni.get().getHttpsUpgradesCount(profile);
    }

    @NativeMethods
    interface Natives {
        void setHTTPSEEnabled(boolean enabled);
        void setAdBlockEnabled(boolean enabled);
        void setFingerprintingProtectionEnabled(boolean enabled);

        void setPlayYTVideoInBrowserEnabled(boolean enabled);
        boolean getPlayYTVideoInBrowserEnabled();

        void setDesktopModeEnabled(boolean enabled);
        boolean getDesktopModeEnabled();

        void setBackgroundVideoPlaybackEnabled(boolean enabled);
        boolean getBackgroundVideoPlaybackEnabled();

        int getTrackersBlockedCount(Profile profile);
        int getAdsBlockedCount(Profile profile);
        int getHttpsUpgradesCount(Profile profile);
    }
}
