/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

import androidx.annotation.NonNull;

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

    public long getTrackersBlockedCount(Profile profile) {
        return BravePrefServiceBridgeJni.get().getTrackersBlockedCount(profile);
    }

    public long getAdsBlockedCount(Profile profile) {
        return BravePrefServiceBridgeJni.get().getAdsBlockedCount(profile);
    }

    public long getDataSaved(Profile profile) {
        return BravePrefServiceBridgeJni.get().getDataSaved(profile);
    }

    /**
     * @param whether SafetyNet check failed.
     */
    public void setSafetynetCheckFailed(boolean value) {
        BravePrefServiceBridgeJni.get().setSafetynetCheckFailed(value);
    }

    public boolean getSafetynetCheckFailed() {
        return BravePrefServiceBridgeJni.get().getSafetynetCheckFailed();
    }

    public void setSafetynetStatus(String status) {
        BravePrefServiceBridgeJni.get().setSafetynetStatus(status);
    }

    public void setUseRewardsStagingServer(boolean enabled) {
        BravePrefServiceBridgeJni.get().setUseRewardsStagingServer(enabled);
    }

    public boolean getUseRewardsStagingServer() {
        return BravePrefServiceBridgeJni.get().getUseRewardsStagingServer();
    }

    public void setOldTrackersBlockedCount(Profile profile, long count) {
        BravePrefServiceBridgeJni.get().setOldTrackersBlockedCount(profile, count);
    }

    public void setOldAdsBlockedCount(Profile profile, long count) {
        BravePrefServiceBridgeJni.get().setOldAdsBlockedCount(profile, count);
    }

    public void setOldHttpsUpgradesCount(Profile profile, long count) {
        BravePrefServiceBridgeJni.get().setOldHttpsUpgradesCount(profile, count);
    }

    public boolean GetBooleanForContentSetting(int content_type) {
        return BravePrefServiceBridgeJni.get().getBooleanForContentSetting(content_type);
    }

    /**
    * @param preference The name of the preference.
    * @return Whether the specified preference is enabled.
    */
    public boolean getBoolean(int preference) {
        return BravePrefServiceBridgeJni.get().getBoolean(preference);
    }

    /**
     * @param preference The name of the preference.
     * @param value The value the specified preference will be set to.
     */
    public void setBoolean(int preference, boolean value) {
        BravePrefServiceBridgeJni.get().setBoolean(preference, value);
    }

    /**
     * @param preference The name of the preference.
     * @return value The value of the specified preference.
     */
    public int getInteger(int preference) {
        return BravePrefServiceBridgeJni.get().getInteger(preference);
    }

    /**
     * @param preference The name of the preference.
     * @param value The value the specified preference will be set to.
     */
    public void setInteger(int preference, int value) {
        BravePrefServiceBridgeJni.get().setInteger(preference, value);
    }

    /**
     * @param preference The name of the preference.
     * @return value The value of the specified preference.
     */
    @NonNull
    public String getString(int preference) {
        return BravePrefServiceBridgeJni.get().getString(preference);
    }

    /**
     * @param preference The name of the preference.
     * @param value The value the specified preference will be set to.
     */
    public void setString(int preference, @NonNull String value) {
        BravePrefServiceBridgeJni.get().setString(preference, value);
    }

    public void setReferralAndroidFirstRunTimestamp(long time) {
        BravePrefServiceBridgeJni.get().setReferralAndroidFirstRunTimestamp(time);
    }

    public void setReferralCheckedForPromoCodeFile(boolean value) {
        BravePrefServiceBridgeJni.get().setReferralCheckedForPromoCodeFile(value);
    }

    public void setReferralInitialization(boolean value) {
        BravePrefServiceBridgeJni.get().setReferralInitialization(value);
    }

    public void setReferralPromoCode(String promoCode) {
        BravePrefServiceBridgeJni.get().setReferralPromoCode(promoCode);
    }

    public void setReferralDownloadId(String downloadId) {
        BravePrefServiceBridgeJni.get().setReferralDownloadId(downloadId);
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

        long getTrackersBlockedCount(Profile profile);
        long getAdsBlockedCount(Profile profile);
        long getDataSaved(Profile profile);

        // Used to pass total stat from upgrading old tabs based browser
        // to a new core based
        void setOldTrackersBlockedCount(Profile profile, long count);
        void setOldAdsBlockedCount(Profile profile, long count);
        void setOldHttpsUpgradesCount(Profile profile, long count);

        void setSafetynetCheckFailed(boolean value);
        boolean getSafetynetCheckFailed();

        void setSafetynetStatus(String status);

        void setUseRewardsStagingServer(boolean enabled);
        boolean getUseRewardsStagingServer();
        boolean getBooleanForContentSetting(int content_type);

        boolean getBoolean(int preference);
        void setBoolean(int preference, boolean value);
        int getInteger(int preference);
        void setInteger(int preference, int value);
        String getString(int preference);
        void setString(int preference, String value);

        void setReferralAndroidFirstRunTimestamp(long time);
        void setReferralCheckedForPromoCodeFile(boolean value);
        void setReferralInitialization(boolean value);
        void setReferralPromoCode(String promoCode);
        void setReferralDownloadId(String downloadId);
    }
}
