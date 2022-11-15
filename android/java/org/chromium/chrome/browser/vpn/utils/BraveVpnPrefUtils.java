/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.utils;

import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.vpn.models.BraveVpnPrefModel;
import org.chromium.components.user_prefs.UserPrefs;

import java.util.Collections;
import java.util.Set;

public class BraveVpnPrefUtils {
    private static final String PREF_BRAVE_VPN_CALLOUT = "brave_vpn_callout";
    private static final String PREF_BRAVE_VPN_CALLOUT_SETTINGS = "brave_vpn_callout_settings";
    private static final String PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE =
            "brave_vpn_subscription_purchase";
    private static final String PREF_BRAVE_VPN_PAYMENT_STATE = "brave_vpn_payment_state";
    private static final String PREF_BRAVE_VPN_HOSTNAME = "brave_vpn_hostname";
    private static final String PREF_BRAVE_VPN_HOSTNAME_DISPLAY = "brave_vpn_hostname_display";
    private static final String PREF_BRAVE_VPN_PURCHASE_TOKEN = "brave_vpn_purchase_token";
    private static final String PREF_BRAVE_VPN_PRODUCT_ID = "brave_vpn_product_id";
    private static final String PREF_BRAVE_VPN_PURCHASE_EXPIRY = "brave_vpn_purchase_expiry";
    private static final String PREF_BRAVE_VPN_SERVER_REGIONS = "brave_vpn_server_regions";
    private static final String PREF_BRAVE_VPN_SERVER_CHANGE_LOCATION = "server_change_location";
    private static final String PREF_BRAVE_VPN_RESET_CONFIGURATION =
            "brave_vpn_reset_configuration";
    private static final String PREF_EXCLUDED_PACKAGES = "excluded_packages";

    public static final String PREF_BRAVE_VPN_AUTOMATIC = "automatic";
    public static final String PREF_BRAVE_VPN_FEATURE = "brave_vpn_feature";
    public static final String PREF_BRAVE_VPN_LINK_SUBSCRIPTION_ON_STAGING =
            "brave_vpn_link_subscription_on_staging";
    public static final String PREF_BRAVE_VPN_START = "brave_vpn_start";

    public static final String PREF_BRAVE_VPN_API_AUTH_TOKEN = "brave_vpn_api_auth_token";
    public static final String PREF_BRAVE_VPN_SUBSCRIBER_CREDENTIAL =
            "brave_vpn_subscriber_credential";
    public static final String PREF_BRAVE_VPN_CLIENT_ID = "brave_vpn_client_id";
    public static final String PREF_BRAVE_VPN_SERVER_PUBLIC_KEY = "brave_vpn_server_public_key";
    public static final String PREF_BRAVE_VPN_IP_ADDRESS = "brave_vpn_ip_address";
    public static final String PREF_BRAVE_VPN_CLIENT_PRIVATE_KEY = "brave_vpn_client_private_key";
    public static final String PREF_SESSION_START_TIME = "brave_vpn_session_start_time";
    public static final String PREF_SESSION_END_TIME = "brave_vpn_session_end_time";
    private static final String PREF_LINK_SUBSCRIPTION_DIALOG = "link_subscription_dialog";

    private static final SharedPreferences mSharedPreferences =
            ContextUtils.getAppSharedPreferences();

    public static boolean isBraveVpnFeatureEnabled() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_VPN_FEATURE, false);
    }

    public static void setBraveVpnFeatureEnabled(boolean newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_FEATURE, newValue);
        sharedPreferencesEditor.apply();
    }

    public static boolean isLinkSubscriptionOnStaging() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_VPN_LINK_SUBSCRIPTION_ON_STAGING, false);
    }

    public static void setLinkSubscriptionOnStaging(boolean newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_LINK_SUBSCRIPTION_ON_STAGING, newValue);
        sharedPreferencesEditor.apply();
    }

    public static boolean isLinkSubscriptionDialogShown() {
        return mSharedPreferences.getBoolean(PREF_LINK_SUBSCRIPTION_DIALOG, false);
    }

    public static void setLinkSubscriptionDialogShown(boolean newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_LINK_SUBSCRIPTION_DIALOG, newValue);
        sharedPreferencesEditor.apply();
    }

    public static boolean shouldShowCallout() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_VPN_CALLOUT, true);
    }

    public static void setCallout(boolean newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_CALLOUT, newValue);
        sharedPreferencesEditor.apply();
    }

    public static boolean shouldShowCalloutSettings() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_VPN_CALLOUT_SETTINGS, true);
    }

    public static void setCalloutSettings(boolean newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_CALLOUT_SETTINGS, newValue);
        sharedPreferencesEditor.apply();
    }

    public static boolean isSubscriptionPurchase() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, false);
    }

    public static void setSubscriptionPurchase(boolean newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, newValue);
        sharedPreferencesEditor.apply();
    }

    public static void setPaymentState(int newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putInt(PREF_BRAVE_VPN_PAYMENT_STATE, newValue);
        sharedPreferencesEditor.apply();
    }

    public static boolean isTrialSubscription() {
        return mSharedPreferences.getInt(PREF_BRAVE_VPN_PAYMENT_STATE, 0) == 2;
    }

    public static boolean isResetConfiguration() {
        return mSharedPreferences.getBoolean(PREF_BRAVE_VPN_RESET_CONFIGURATION, false);
    }

    public static void setResetConfiguration(boolean newValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_RESET_CONFIGURATION, newValue);
        sharedPreferencesEditor.apply();
    }

    public static void setHostname(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_HOSTNAME, value);
        sharedPreferencesEditor.apply();
    }

    public static String getHostname() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_HOSTNAME, "");
    }

    public static void setHostnameDisplay(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_HOSTNAME_DISPLAY, value);
        sharedPreferencesEditor.apply();
    }

    public static String getHostnameDisplay() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_HOSTNAME_DISPLAY, "");
    }

    public static void setPurchaseToken(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_PURCHASE_TOKEN, value);
        sharedPreferencesEditor.apply();
        UserPrefs.get(Profile.getLastUsedRegularProfile())
                .setString(BravePref.BRAVE_VPN_PURCHASE_TOKEN_ANDROID, value);
        UserPrefs.get(Profile.getLastUsedRegularProfile())
                .setString(BravePref.BRAVE_VPN_PACKAGE_ANDROID,
                        ContextUtils.getApplicationContext().getPackageName());
    }

    public static String getPurchaseToken() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_PURCHASE_TOKEN, "");
    }

    public static void setProductId(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_PRODUCT_ID, value);
        sharedPreferencesEditor.apply();
        UserPrefs.get(Profile.getLastUsedRegularProfile())
                .setString(BravePref.BRAVE_VPN_PRODUCT_ID_ANDROID, value);
    }

    public static String getProductId() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_PRODUCT_ID, "");
    }

    public static void setPurchaseExpiry(Long value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(PREF_BRAVE_VPN_PURCHASE_EXPIRY, value);
        sharedPreferencesEditor.apply();
    }

    public static Long getPurchaseExpiry() {
        return mSharedPreferences.getLong(PREF_BRAVE_VPN_PURCHASE_EXPIRY, 0);
    }

    public static void setServerRegions(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_SERVER_REGIONS, value);
        sharedPreferencesEditor.apply();
    }

    public static String getServerRegions() {
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

    public static void setApiAuthToken(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_API_AUTH_TOKEN, value);
        sharedPreferencesEditor.apply();
    }

    public static String getApiAuthToken() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_API_AUTH_TOKEN, "");
    }

    public static void setSubscriberCredential(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_SUBSCRIBER_CREDENTIAL, value);
        sharedPreferencesEditor.apply();
    }

    public static String getSubscriberCredential() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_SUBSCRIBER_CREDENTIAL, "");
    }

    public static void setClientId(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_CLIENT_ID, value);
        sharedPreferencesEditor.apply();
    }

    public static String getClientId() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_CLIENT_ID, "");
    }

    public static void setIpAddress(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_IP_ADDRESS, value);
        sharedPreferencesEditor.apply();
    }

    public static String getIpAddress() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_IP_ADDRESS, "");
    }

    public static void setServerPublicKey(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_SERVER_PUBLIC_KEY, value);
        sharedPreferencesEditor.apply();
    }

    public static String getServerPublicKey() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_SERVER_PUBLIC_KEY, "");
    }

    public static void setClientPrivateKey(String value) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(PREF_BRAVE_VPN_CLIENT_PRIVATE_KEY, value);
        sharedPreferencesEditor.apply();
    }

    public static String getClientPrivateKey() {
        return mSharedPreferences.getString(PREF_BRAVE_VPN_CLIENT_PRIVATE_KEY, "");
    }

    public static void setPrefModel(BraveVpnPrefModel braveVpnPrefModel) {
        setHostname(braveVpnPrefModel.getHostname());
        setHostnameDisplay(braveVpnPrefModel.getHostnameDisplay());
        setServerRegion(braveVpnPrefModel.getServerRegion());
        setPurchaseToken(braveVpnPrefModel.getPurchaseToken());
        setProductId(braveVpnPrefModel.getProductId());
        setSubscriberCredential(braveVpnPrefModel.getSubscriberCredential());
        setClientId(braveVpnPrefModel.getClientId());
        setApiAuthToken(braveVpnPrefModel.getApiAuthToken());
        setResetConfiguration(false);
    }

    public static Set<String> getExcludedPackages() {
        return mSharedPreferences.getStringSet(PREF_EXCLUDED_PACKAGES, Collections.emptySet());
    }

    public static void setExcludedPackages(Set<String> packages) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putStringSet(PREF_EXCLUDED_PACKAGES, packages);
        sharedPreferencesEditor.apply();
    }

    public static void setSessionEndTimeMs(long timeMs) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(PREF_SESSION_END_TIME, timeMs);
        sharedPreferencesEditor.apply();
    }

    public static long getSessionEndTimeMs() {
        return mSharedPreferences.getLong(PREF_SESSION_END_TIME, -1);
    }

    public static void setSessionStartTimeMs(long timeMs) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putLong(PREF_SESSION_START_TIME, timeMs);
        sharedPreferencesEditor.apply();
    }

    public static long getSessionStartTimeMs() {
        return mSharedPreferences.getLong(PREF_SESSION_START_TIME, -1);
    }
}
