/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.preferences.website;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JCaller;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.base.Log;
import org.chromium.chrome.browser.ContentSettingsType;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettingsObserver;
import org.chromium.chrome.browser.preferences.website.WebsitePreferenceBridge;

import java.util.List;

@JNINamespace("chrome::android")
public class BraveShieldsContentSettings {
    static public final String RESOURCE_IDENTIFIER_ADS = "ads";
    static public final String RESOURCE_IDENTIFIER_TRACKERS = "trackers";
    static public final String RESOURCE_IDENTIFIER_ADS_TRACKERS = "ads_trackers";
    static public final String RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES = "httpUpgradableResources";
    static public final String RESOURCE_IDENTIFIER_BRAVE_SHIELDS = "braveShields";
    static public final String RESOURCE_IDENTIFIER_FINGERPRINTING = "fingerprinting";
    static public final String RESOURCE_IDENTIFIER_COOKIES = "cookies";
    static public final String RESOURCE_IDENTIFIER_REFERRERS = "referrers";
    static public final String RESOURCE_IDENTIFIER_JAVASCRIPTS = "javascript";

    static private final String blockResource = "block";
    static private final String allowResource = "allow";

    private long mNativeBraveShieldsContentSettings;
    private BraveShieldsContentSettingsObserver mBraveShieldsContentSettingsObserver;

    public BraveShieldsContentSettings(BraveShieldsContentSettingsObserver braveShieldsContentSettingsObserver) {
        mBraveShieldsContentSettingsObserver = braveShieldsContentSettingsObserver;
        mNativeBraveShieldsContentSettings = 0;
        init();
    }

    private void init() {
        if (mNativeBraveShieldsContentSettings == 0) {
            BraveShieldsContentSettingsJni.get().init(this);
        }
    }

    public void destroy() {
        if (mNativeBraveShieldsContentSettings == 0) {
            return;
        }
        BraveShieldsContentSettingsJni.get().destroy(mNativeBraveShieldsContentSettings);
    }

    static public void setShields(boolean incognito, String host, String resourceIndentifier, boolean value,
            boolean fromTopShields) {
        String setting_string = (value ? blockResource : allowResource);
        if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
            BraveShieldsContentSettingsJni.get().setBraveShieldsControlType(setting_string, host);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_ADS_TRACKERS)) {
            BraveShieldsContentSettingsJni.get().setAdControlType(setting_string, host);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES)) {
            BraveShieldsContentSettingsJni.get().setHTTPSEverywhereControlType(setting_string, host);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_COOKIES)) {
            BraveShieldsContentSettingsJni.get().setCookieControlType(setting_string, host);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_FINGERPRINTING)) {
            BraveShieldsContentSettingsJni.get().setFingerprintingControlType(setting_string, host);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_JAVASCRIPTS)) {
            BraveShieldsContentSettingsJni.get().setNoScriptControlType(setting_string, host);
        }
    }

    public static boolean getShields(boolean incognito, String host, String resourceIndentifier) {
        String settings = blockResource;
        if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_BRAVE_SHIELDS)) {
            settings = BraveShieldsContentSettingsJni.get().getBraveShieldsControlType(host);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_ADS_TRACKERS)) {
            settings = BraveShieldsContentSettingsJni.get().getAdControlType(host);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES)) {
            settings = BraveShieldsContentSettingsJni.get().getHTTPSEverywhereControlType(host);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_COOKIES)) {
            settings = BraveShieldsContentSettingsJni.get().getCookieControlType(host);
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_FINGERPRINTING)) {
            settings = BraveShieldsContentSettingsJni.get().getFingerprintingControlType(host);
            if (!settings.equals(allowResource) && !settings.equals(blockResource)) {
                return false;
            }
        } else if (resourceIndentifier.equals(RESOURCE_IDENTIFIER_JAVASCRIPTS)) {
            settings = BraveShieldsContentSettingsJni.get().getNoScriptControlType(host);
        }

        return !settings.equals(allowResource); 
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveShieldsContentSettings == 0;
        mNativeBraveShieldsContentSettings = nativePtr;
    }

    @CalledByNative
    private void blockedEvent(int tabId, String block_type, String subresource) {
        assert mBraveShieldsContentSettingsObserver != null;
        if (mBraveShieldsContentSettingsObserver == null) {
            return;
        }
        mBraveShieldsContentSettingsObserver.blockEvent(tabId, block_type, subresource);
    }

    @NativeMethods
    interface Natives {
        void init(@JCaller BraveShieldsContentSettings self);
        void destroy(long nativeBraveShieldsContentSettings);

        void setBraveShieldsControlType(String type, String url);
        String getBraveShieldsControlType(String url);
        void setAdControlType(String type, String url);
        String getAdControlType(String url);
        void setCookieControlType(String type, String url);
        String getCookieControlType(String url);
        void setFingerprintingControlType(String type, String url);
        String getFingerprintingControlType(String url);
        void setHTTPSEverywhereControlType(String type, String url);
        String getHTTPSEverywhereControlType(String url);
        void setNoScriptControlType(String type, String url);
        String getNoScriptControlType(String url);
    }
}
