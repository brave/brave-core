/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences;

import androidx.annotation.NonNull;

import org.chromium.base.Log;
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
     * @param whether the IPFS gateway should be enabled.
     */
    public void setIpfsGatewayEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setIpfsGatewayEnabled(enabled);
    }

    /**
     * @param whether google login is enabled on third party sites.
     */
    public void setThirdPartyGoogleLoginEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setThirdPartyGoogleLoginEnabled(enabled);
    }

    /**
     * @param whether facebook embeds are allowed on third party sites.
     */
    public void setThirdPartyFacebookEmbedEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setThirdPartyFacebookEmbedEnabled(enabled);
    }

    /**
     * @param whether twitter embeds are allowed on third party sites.
     */
    public void setThirdPartyTwitterEmbedEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setThirdPartyTwitterEmbedEnabled(enabled);
    }

    /**
     * @param whether linkedin embeds are allowed on third party sites.
     */
    public void setThirdPartyLinkedinEmbedEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setThirdPartyLinkedinEmbedEnabled(enabled);
    }

    /**
     * @param whether AdBlock should be enabled.
     */
    public void setAdBlockEnabled(boolean enabled) {
        BravePrefServiceBridgeJni.get().setAdBlockEnabled(enabled);
    }

    public void setNoScriptControlType(String type) {
        BravePrefServiceBridgeJni.get().setNoScriptControlType(type);
    }

    public String getNoScriptControlType() {
        return BravePrefServiceBridgeJni.get().getNoScriptControlType();
    }

    public void setCookiesBlockType(String type) {
        BravePrefServiceBridgeJni.get().setCookiesBlockType(type);
    }

    public String getCookiesBlockType() {
        return BravePrefServiceBridgeJni.get().getCookiesBlockType();
    }

    /**
     * @param whether Fingerprinting Protection should be enabled.
     */

    public void setFingerprintingControlType(String type) {
        BravePrefServiceBridgeJni.get().setFingerprintingControlType(type);
    }

    public String getFingerprintingControlType() {
        return BravePrefServiceBridgeJni.get().getFingerprintingControlType();
    }

    public void setCosmeticFilteringControlType(int type) {
        BravePrefServiceBridgeJni.get().setCosmeticFilteringControlType(type);
    }

    public String getCosmeticFilteringControlType() {
        return BravePrefServiceBridgeJni.get().getCosmeticFilteringControlType();
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

    public void resetPromotionLastFetchStamp() {
        BravePrefServiceBridgeJni.get().resetPromotionLastFetchStamp();
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

    public void setP3AEnabled(boolean value) {
        BravePrefServiceBridgeJni.get().setP3AEnabled(value);
    }

    public boolean getP3AEnabled() {
        return BravePrefServiceBridgeJni.get().getP3AEnabled();
    }

    public boolean hasPathP3AEnabled() {
        return BravePrefServiceBridgeJni.get().hasPathP3AEnabled();
    }

    public void setP3ANoticeAcknowledged(boolean value) {
        BravePrefServiceBridgeJni.get().setP3ANoticeAcknowledged(value);
    }

    public boolean getP3ANoticeAcknowledged() {
        return BravePrefServiceBridgeJni.get().getP3ANoticeAcknowledged();
    }

    public void setUnstoppableDomainsResolveMethod(int method) {
        BravePrefServiceBridgeJni.get().setUnstoppableDomainsResolveMethod(method);
    }

    public int getUnstoppableDomainsResolveMethod() {
        return BravePrefServiceBridgeJni.get().getUnstoppableDomainsResolveMethod();
    }

    public void setENSResolveMethod(int method) {
        BravePrefServiceBridgeJni.get().setENSResolveMethod(method);
    }

    public int getENSResolveMethod() {
        return BravePrefServiceBridgeJni.get().getENSResolveMethod();
    }

    public void setWebrtcPolicy(int policy) {
        BravePrefServiceBridgeJni.get().setWebrtcPolicy(policy);
    }

    public int getWebrtcPolicy() {
        return BravePrefServiceBridgeJni.get().getWebrtcPolicy();
    }

    @NativeMethods
    interface Natives {
        void setCosmeticFilteringControlType(int type);
        String getCosmeticFilteringControlType();

        void setCookiesBlockType(String type);
        String getCookiesBlockType();

        void setFingerprintingControlType(String type);
        String getFingerprintingControlType();

        void setNoScriptControlType(String type);
        String getNoScriptControlType();

        void setHTTPSEEnabled(boolean enabled);
        void setIpfsGatewayEnabled(boolean enabled);
        void setAdBlockEnabled(boolean enabled);

        void setThirdPartyGoogleLoginEnabled(boolean enabled);
        void setThirdPartyFacebookEmbedEnabled(boolean enabled);
        void setThirdPartyTwitterEmbedEnabled(boolean enabled);
        void setThirdPartyLinkedinEmbedEnabled(boolean enabled);

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
        void resetPromotionLastFetchStamp();
        boolean getUseRewardsStagingServer();
        boolean getBooleanForContentSetting(int content_type);

        void setReferralAndroidFirstRunTimestamp(long time);
        void setReferralCheckedForPromoCodeFile(boolean value);
        void setReferralInitialization(boolean value);
        void setReferralPromoCode(String promoCode);
        void setReferralDownloadId(String downloadId);

        void setP3AEnabled(boolean value);
        boolean getP3AEnabled();
        boolean hasPathP3AEnabled();
        void setP3ANoticeAcknowledged(boolean value);
        boolean getP3ANoticeAcknowledged();

        void setUnstoppableDomainsResolveMethod(int method);
        void setENSResolveMethod(int method);
        int getUnstoppableDomainsResolveMethod();
        int getENSResolveMethod();

        void setWebrtcPolicy(int policy);
        int getWebrtcPolicy();
    }
}
