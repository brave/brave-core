/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.profiles.Profile;

/**
 * Please don't add anything in that file. We are going to refactor it soon.
 * Check this PRs on how to handle preferences correctly:
 * https://github.com/brave/brave-core/pull/16356
 * https://github.com/brave/brave-core/pull/15905
 * For the local_state based prefs please look on the PR:
 * https://github.com/brave/brave-core/pull/16486
 * Contact code owners if you have additional questions.
 */

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

    public void setCookiesBlockType(String type) {
        BravePrefServiceBridgeJni.get().setCookiesBlockType(type);
    }

    public String getCookiesBlockType() {
        return BravePrefServiceBridgeJni.get().getCookiesBlockType();
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

    public void resetPromotionLastFetchStamp() {
        BravePrefServiceBridgeJni.get().resetPromotionLastFetchStamp();
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

    public void setWebrtcPolicy(int policy) {
        BravePrefServiceBridgeJni.get().setWebrtcPolicy(policy);
    }

    public int getWebrtcPolicy() {
        return BravePrefServiceBridgeJni.get().getWebrtcPolicy();
    }

    public void setNewsOptIn(boolean value) {
        BravePrefServiceBridgeJni.get().setNewsOptIn(value);
    }

    public boolean getNewsOptIn() {
        return BravePrefServiceBridgeJni.get().getNewsOptIn();
    }

    public void setShowNews(boolean value) {
        BravePrefServiceBridgeJni.get().setShowNews(value);
    }

    public boolean getShowNews() {
        return BravePrefServiceBridgeJni.get().getShowNews();
    }

    @NativeMethods
    interface Natives {
        void setCookiesBlockType(String type);
        String getCookiesBlockType();

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

        void resetPromotionLastFetchStamp();
        boolean getBooleanForContentSetting(int content_type);

        void setWebrtcPolicy(int policy);
        int getWebrtcPolicy();

        void setNewsOptIn(boolean value);
        boolean getNewsOptIn();

        void setShowNews(boolean value);
        boolean getShowNews();
    }
}
