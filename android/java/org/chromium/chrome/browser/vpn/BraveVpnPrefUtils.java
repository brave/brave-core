/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;

public class BraveVpnPrefUtils {
    public static final String PREF_BRAVE_VPN_FEATURE = "brave_vpn_feature";
    public static final String PREF_BRAVE_VPN_CALLOUT = "brave_vpn_callout";
    public static final String PREF_BRAVE_VPN_CALLOUT_SETTINGS = "brave_vpn_callout_settings";
    public static final String PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE =
            "brave_vpn_subscription_purchase";
    public static final String PREF_BRAVE_VPN_HOSTNAME = "brave_vpn_hostname";
    public static final String PREF_BRAVE_VPN_PURCHASE_TOKEN = "brave_vpn_purchase_token";
    public static final String PREF_BRAVE_VPN_PRODUCT_ID = "brave_vpn_product_id";
    public static final String PREF_BRAVE_VPN_PURCHASE_EXPIRY = "brave_vpn_purchase_expiry";
    public static final String PREF_BRAVE_VPN_SERVER_REGIONS = "brave_vpn_server_regions";

    public static boolean isBraveVpnBooleanPref(String pref, boolean defaultValue) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(pref, defaultValue);
    }

    public static void setBraveVpnBooleanPref(String pref, boolean newValue) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(pref, newValue);
        sharedPreferencesEditor.apply();
    }

    public static void setBraveVpnStringPref(String pref, String value) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putString(pref, value);
        sharedPreferencesEditor.apply();
    }

    public static String getBraveVpnStringPref(String pref) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getString(pref, "");
    }

    public static String getServerRegion(String serverRegionPref) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getString(serverRegionPref, "automatic");
    }

    public static void setServerRegion(String serverRegionPref, String newValue) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putString(serverRegionPref, newValue);
        sharedPreferencesEditor.apply();
    }
}
