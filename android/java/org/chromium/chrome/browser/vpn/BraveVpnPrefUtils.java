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
    private static final String PREF_BRAVE_VPN_CALLOUT = "brave_vpn_callout";
    private static final String PREF_BRAVE_VPN_CALLOUT_SETTINGS = "brave_vpn_callout_settings";
    private static final String PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE =
            "brave_vpn_subscription_purchase";
    private static final String PREF_BRAVE_VPN_HOSTNAME = "brave_vpn_hostname";
    private static final String PREF_BRAVE_VPN_PURCHASE_TOKEN = "brave_vpn_purchase_token";
    private static final String PREF_BRAVE_VPN_PRODUCT_ID = "brave_vpn_product_id";
    private static final String PREF_BRAVE_VPN_PURCHASE_EXPIRY = "brave_vpn_purchase_expiry";
    private static final String PREF_BRAVE_VPN_SERVER_REGIONS = "brave_vpn_server_regions";
    private static final String PREF_BRAVE_VPN_SERVER_CHANGE_LOCATION = "server_change_location";

    public static final String PREF_BRAVE_VPN_AUTOMATIC = "automatic";
    public static final String PREF_BRAVE_VPN_FEATURE = "brave_vpn_feature";

    private static final SharedPreferences mSharedPreferences =
            ContextUtils.getAppSharedPreferences();

    public static boolean isBraveVpnFeatureEnable() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_VPN_FEATURE, false);
    }

    public static void setBraveVpnFeatureEnable(boolean newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_FEATURE, newValue);
        sharedPreferencesEditor.apply();
    }

    public static boolean shouldShowBraveVpnCallout() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_VPN_CALLOUT, true);
    }

    public static void setBraveVpnCallout(boolean newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_CALLOUT, newValue);
        sharedPreferencesEditor.apply();
    }

    public static boolean shouldShowBraveVpnCalloutSettings() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_VPN_CALLOUT_SETTINGS, true);
    }

    public static void setBraveVpnCalloutSettings(boolean newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_CALLOUT_SETTINGS, newValue);
        sharedPreferencesEditor.apply();
    }

    public static boolean isBraveVpnSubscriptionPurchase() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, false);
    }

    public static void setBraveVpnSubscriptionPurchase(boolean newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, newValue);
        sharedPreferencesEditor.apply();
    }

    public static void setBraveVpnHostname(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_HOSTNAME, value);
        sharedPreferencesEditor.apply();
    }

    public static String getBraveVpnHostname() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_HOSTNAME, "");
    }

    public static void setBraveVpnPurchaseToken(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_PURCHASE_TOKEN, value);
        sharedPreferencesEditor.apply();
    }

    public static String getBraveVpnPurchaseToken() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_PURCHASE_TOKEN, "");
    }

    public static void setBraveVpnProductId(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_PRODUCT_ID, value);
        sharedPreferencesEditor.apply();
    }

    public static String getBraveVpnProductId() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_PRODUCT_ID, "");
    }

    public static void setBraveVpnPurchaseExpiry(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_PURCHASE_EXPIRY, value);
        sharedPreferencesEditor.apply();
    }

    public static String getBraveVpnPurchaseExpiry() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_PURCHASE_EXPIRY, "");
    }

    public static void setBraveVpnServerRegions(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_SERVER_REGIONS, value);
        sharedPreferencesEditor.apply();
    }

    public static String getBraveVpnServerRegions() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_SERVER_REGIONS, "");
    }

    public static String getServerRegion() {
        return mSharedPreferences.getString(
                PREF_BRAVE_VPN_SERVER_CHANGE_LOCATION, PREF_BRAVE_VPN_AUTOMATIC);
    }

    public static void setServerRegion(String newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_SERVER_CHANGE_LOCATION, newValue);
        sharedPreferencesEditor.apply();
    }
}
