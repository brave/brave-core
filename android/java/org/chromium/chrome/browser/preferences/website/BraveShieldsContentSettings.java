/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.website;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;

import java.util.ArrayList;
import java.util.List;

@JNINamespace("chrome::android")
public class BraveShieldsContentSettings {
    static public final String RESOURCE_IDENTIFIER_ADS = "shieldsAds";
    static public final String RESOURCE_IDENTIFIER_TRACKERS = "trackers";
    static public final String RESOURCE_IDENTIFIER_DATA_SAVED = "data_saved";
    static public final String RESOURCE_IDENTIFIER_BRAVE_SHIELDS = "braveShields";
    static public final String RESOURCE_IDENTIFIER_FINGERPRINTING = "fingerprinting";
    static public final String RESOURCE_IDENTIFIER_COOKIES = "shieldsCookies";
    static public final String RESOURCE_IDENTIFIER_REFERRERS = "referrers";
    static public final String RESOURCE_IDENTIFIER_JAVASCRIPTS = "javascript";
    static public final String RESOURCE_IDENTIFIER_HTTPS_UPGRADE = "httpsUpgrade";
    static public final String RESOURCE_IDENTIFIER_FORGET_FIRST_PARTY_STORAGE =
            "forgetFirstPartyStorage";

    static public final String BLOCK_RESOURCE = "block";
    static public final String BLOCK_THIRDPARTY_RESOURCE = "block_third_party";
    static public final String DEFAULT = "default";
    static public final String ALLOW_RESOURCE = "allow";
    static public final String AGGRESSIVE = "aggressive";

    public static final int ALWAYS = 0;
    public static final int ASK = 1;
    public static final int NEVER = 2;

    private long mNativeBraveShieldsContentSettings;
    private List<BraveShieldsContentSettingsObserver> mBraveShieldsContentSettingsObservers;
    private static BraveShieldsContentSettings sInstance;

    public static BraveShieldsContentSettings getInstance() {
        if (sInstance == null) sInstance = new BraveShieldsContentSettings();
        return sInstance;
    }

    private BraveShieldsContentSettings() {
        mNativeBraveShieldsContentSettings = 0;
        mBraveShieldsContentSettingsObservers =
            new ArrayList<BraveShieldsContentSettingsObserver>();
        init();
    }

    public void addObserver(BraveShieldsContentSettingsObserver observer) {
        mBraveShieldsContentSettingsObservers.add(observer);
    }

    public void removeObserver(BraveShieldsContentSettingsObserver observer) {
        mBraveShieldsContentSettingsObservers.remove(observer);
    }

    private void init() {
        if (mNativeBraveShieldsContentSettings == 0) {
            BraveShieldsContentSettingsJni.get().init(this);
        }
    }

    /**
     * A finalizer is required to ensure that the native object associated with this descriptor gets
     * torn down, otherwise there would be a memory leak.
     */
    @SuppressWarnings("Finalize")
    @Override
    protected void finalize() {
        destroy();
    }

    private void destroy() {
        if (mNativeBraveShieldsContentSettings == 0) {
            return;
        }
        BraveShieldsContentSettingsJni.get().destroy(mNativeBraveShieldsContentSettings);
        mNativeBraveShieldsContentSettings = 0;
    }

    public static void setShields(
            Profile profile,
            String host,
            String resourceIndentifier,
            boolean value,
            boolean fromTopShields) {
        String setting_string = (value ? BLOCK_RESOURCE : ALLOW_RESOURCE);
        if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
            BraveShieldsContentSettingsJni.get().setBraveShieldsEnabled(value, host, profile);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_JAVASCRIPTS)) {
            BraveShieldsContentSettingsJni.get().setNoScriptControlType(setting_string, host, profile);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_FORGET_FIRST_PARTY_STORAGE)) {
            BraveShieldsContentSettingsJni.get().setForgetFirstPartyStorageEnabled(
                    value, host, profile);
        }
    }

    public static void setShieldsValue(Profile profile, String host, String resourceIndentifier,
            String settingOption, boolean fromTopShields) {
        if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_FINGERPRINTING)) {
            BraveShieldsContentSettingsJni.get().setFingerprintingControlType(
                    settingOption, host, profile);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_HTTPS_UPGRADE)) {
            BraveShieldsContentSettingsJni.get().setHttpsUpgradeControlType(
                    settingOption, host, profile);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_COOKIES)) {
            BraveShieldsContentSettingsJni.get().setCookieControlType(settingOption, host, profile);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_TRACKERS)) {
            BraveShieldsContentSettingsJni.get().setCosmeticFilteringControlType(
                    DEFAULT.equals(settingOption) ? BLOCK_THIRDPARTY_RESOURCE : settingOption, host,
                    profile);
            BraveShieldsContentSettingsJni.get().setAdControlType(
                    BLOCK_THIRDPARTY_RESOURCE.equals(settingOption) ? DEFAULT : settingOption, host,
                    profile);
        }
    }

    public static boolean getShields(Profile profile, String host, String resourceIndentifier) {
        String settings = BLOCK_RESOURCE;
        if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
            return BraveShieldsContentSettingsJni.get().getBraveShieldsEnabled(host, profile);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_JAVASCRIPTS)) {
            settings = BraveShieldsContentSettingsJni.get().getNoScriptControlType(host, profile);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_FORGET_FIRST_PARTY_STORAGE)) {
            return BraveShieldsContentSettingsJni.get().getForgetFirstPartyStorageEnabled(
                    host, profile);
        }

        return !settings.equals(ALLOW_RESOURCE);
    }

    public static String getShieldsValue(Profile profile, String host, String resourceIndentifier) {
        String settings = BLOCK_RESOURCE;
        if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_FINGERPRINTING)) {
            settings = BraveShieldsContentSettingsJni.get().getFingerprintingControlType(
                    host, profile);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_HTTPS_UPGRADE)) {
            settings =
                    BraveShieldsContentSettingsJni.get().getHttpsUpgradeControlType(host, profile);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_COOKIES)) {
            settings = BraveShieldsContentSettingsJni.get().getCookieControlType(host, profile);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_TRACKERS)) {
            settings = BraveShieldsContentSettingsJni.get().getCosmeticFilteringControlType(
                    host, profile);
            if (settings.equals(BLOCK_THIRDPARTY_RESOURCE)) {
                settings = DEFAULT;
            }
        }
        return settings;
    }

    public static void setFingerprintingPref(String value) {
        setShieldsValue(
                ProfileManager.getLastUsedRegularProfile(),
                "",
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING,
                value,
                false);
    }

    public static void setHttpsUpgradePref(String value) {
        setShieldsValue(
                ProfileManager.getLastUsedRegularProfile(),
                "",
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_HTTPS_UPGRADE,
                value,
                false);
    }

    public static void setCookiesPref(String value) {
        setShieldsValue(
                ProfileManager.getLastUsedRegularProfile(),
                "",
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_COOKIES,
                value,
                false);
    }

    public static void setTrackersPref(String value) {
        setShieldsValue(
                ProfileManager.getLastUsedRegularProfile(),
                "",
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS,
                value,
                false);
    }

    public static void setJavascriptPref(boolean value) {
        setShields(
                ProfileManager.getLastUsedRegularProfile(),
                "",
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_JAVASCRIPTS,
                value,
                false);
    }

    public static void setForgetFirstPartyStoragePref(boolean value) {
        setShields(
                ProfileManager.getLastUsedRegularProfile(),
                "",
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FORGET_FIRST_PARTY_STORAGE,
                value,
                false);
    }

    public static boolean getJavascriptPref() {
        return getShields(
                ProfileManager.getLastUsedRegularProfile(),
                "",
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_JAVASCRIPTS);
    }

    public static String getTrackersPref() {
        return getShieldsValue(
                ProfileManager.getLastUsedRegularProfile(),
                "",
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_TRACKERS);
    }

    public static String getFingerprintingPref() {
        return getShieldsValue(
                ProfileManager.getLastUsedRegularProfile(),
                "",
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FINGERPRINTING);
    }

    public static String getHttpsUpgradePref() {
        return getShieldsValue(
                ProfileManager.getLastUsedRegularProfile(),
                "",
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_HTTPS_UPGRADE);
    }

    public static boolean getForgetFirstPartyStoragePref() {
        return getShields(
                ProfileManager.getLastUsedRegularProfile(),
                "",
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_FORGET_FIRST_PARTY_STORAGE);
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveShieldsContentSettings == 0;
        mNativeBraveShieldsContentSettings = nativePtr;
    }

    @CalledByNative
    private void blockedEvent(int tabId, String block_type, String subresource) {
        for (BraveShieldsContentSettingsObserver observer : mBraveShieldsContentSettingsObservers) {
            observer.blockEvent(tabId, block_type, subresource);
        }
    }

    @CalledByNative
    private void savedBandwidth(long savings) {
        for (BraveShieldsContentSettingsObserver observer : mBraveShieldsContentSettingsObservers) {
            observer.savedBandwidth(savings);
        }
    }

    @NativeMethods
    interface Natives {
        void init(BraveShieldsContentSettings self);
        void destroy(long nativeBraveShieldsContentSettings);

        void setBraveShieldsEnabled(boolean enabled, String url, Profile profile);
        boolean getBraveShieldsEnabled(String url, Profile profile);
        void setAdControlType(String type, String url, Profile profile);
        String getAdControlType(String url, Profile profile);
        void setCookieControlType(String type, String url, Profile profile);
        String getCookieControlType(String url, Profile profile);
        void setFingerprintingControlType(String type, String url, Profile profile);
        String getFingerprintingControlType(String url, Profile profile);
        void setHttpsUpgradeControlType(String type, String url, Profile profile);
        String getHttpsUpgradeControlType(String url, Profile profile);
        void setNoScriptControlType(String type, String url, Profile profile);
        String getNoScriptControlType(String url, Profile profile);
        void setForgetFirstPartyStorageEnabled(boolean enabled, String url, Profile profile);
        boolean getForgetFirstPartyStorageEnabled(String url, Profile profile);

        void setCosmeticFilteringControlType(String type, String url, Profile profile);
        String getCosmeticFilteringControlType(String url, Profile profile);
    }
}
