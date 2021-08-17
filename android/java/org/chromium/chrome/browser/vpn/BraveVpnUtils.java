/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.Ikev2VpnProfile;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.VpnManager;
import android.os.Build;
import android.util.Pair;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.vpn.BraveVpnPlansActivity;
import org.chromium.chrome.browser.vpn.BraveVpnProfileActivity;
import org.chromium.chrome.browser.vpn.VpnServerRegion;

import java.util.ArrayList;
import java.util.List;

public class BraveVpnUtils {
    public static final int MONTHLY_SUBSCRIPTION = 1;
    public static final int YEARLY_SUBSCRIPTION = 2;

    private static final String PREF_BRAVE_VPN_CALLOUT = "brave_vpn_callout";
    private static final String PREF_BRAVE_VPN_CALLOUT_SETTINGS = "brave_vpn_callout_settings";
    private static final String PREF_BRAVE_SUBSCRIPTION_PURCHASE = "brave_subscription_purchase";

    public static List<VpnServerRegion> vpnServerRegions = new ArrayList<>();

    public static boolean shouldShowVpnCalloutView() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(PREF_BRAVE_VPN_CALLOUT, true);
    }

    public static void setShowVpnCalloutView() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_CALLOUT, false);
        sharedPreferencesEditor.apply();
    }

    public static boolean isSubscriptionPurchased() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(PREF_BRAVE_SUBSCRIPTION_PURCHASE, false);
    }

    public static void setSubscriptionPurchased() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_SUBSCRIPTION_PURCHASE, true);
        sharedPreferencesEditor.apply();
    }

    public static boolean shouldShowVpnCalloutSettingsView() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getBoolean(PREF_BRAVE_VPN_CALLOUT_SETTINGS, true);
    }

    public static void setShowVpnCalloutSettingsView() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_VPN_CALLOUT_SETTINGS, false);
        sharedPreferencesEditor.apply();
    }

    public static String getServerRegion(String serverRegionPref) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        return sharedPreferences.getString(serverRegionPref, "automatic");
    }

    public static boolean isBraveVpnFeatureEnable() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return true;
        }
        return false;
    }

    public static void openBraveVpnPlansActivity(Activity activity) {
        Intent braveVpnPlanIntent = new Intent(activity, BraveVpnPlansActivity.class);
        braveVpnPlanIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        activity.startActivity(braveVpnPlanIntent);
    }

    public static void openBraveVpnProfileActivity(Context context) {
        Intent braveVpnProfileIntent = new Intent(context, BraveVpnProfileActivity.class);
        braveVpnProfileIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        context.startActivity(braveVpnProfileIntent);
    }

    public static String getRegionForTimeZone(String jsonTimezones, String currentTimezone) {
        // Add root element to make it real JSON, otherwise getJSONArray cannot parse it
        jsonTimezones = "{\"regions\":" + jsonTimezones + "}";
        try {
            JSONObject result = new JSONObject(jsonTimezones);
            JSONArray regions = result.getJSONArray("regions");
            for (int i = 0; i < regions.length(); i++) {
                JSONObject region = regions.getJSONObject(i);
                JSONArray timezones = region.getJSONArray("timezones");
                for (int j = 0; j < timezones.length(); j++) {
                    if (timezones.getString(j).equals(currentTimezone)) {
                        return region.getString("name");
                    }
                }
            }
        } catch (JSONException e) {
            Log.e("BraveVPN", "getRegionForTimeZone JSONException error " + e);
        }
        return "";
    }

    public static String getHostnameForRegion(String jsonHostnames) {
        jsonHostnames = "{\"hostnames\":" + jsonHostnames + "}";
        try {
            JSONObject result = new JSONObject(jsonHostnames);
            JSONArray hostnames = result.getJSONArray("hostnames");
            JSONObject hostname = hostnames.getJSONObject(0);
            return hostname.getString("hostname");
        } catch (JSONException e) {
            Log.e("BraveVPN", "getHostnameForRegion JSONException error " + e);
        }
        return "";
    }

    public static Pair<String, String> getProfileCredentials(String jsonProfileCredentials) {
        try {
            JSONObject profileCredentials = new JSONObject(jsonProfileCredentials);
            return new Pair<>(profileCredentials.getString("eap-username"),
                    profileCredentials.getString("eap-password"));
        } catch (JSONException e) {
            Log.e("BraveVPN", "getProfileCredentials JSONException error " + e);
        }
        return null;
    }

    public static boolean isPurchaseValid(String json) {
        try {
            JSONObject purchase = new JSONObject(json);
            return Long.parseLong(purchase.getString("expiryTimeMillis"))
                    >= System.currentTimeMillis();
        } catch (JSONException e) {
            Log.e("BraveVPN", "getProfileCredentials JSONException error " + e);
        }
        return false;
    }

    public static void getServerLocations(String jsonServerLocations) {
        vpnServerRegions.clear();
        vpnServerRegions.add(new VpnServerRegion("automatic", "automatic", "Automatic"));
        jsonServerLocations = "{\"servers\":" + jsonServerLocations + "}";
        try {
            JSONObject result = new JSONObject(jsonServerLocations);
            JSONArray servers = result.getJSONArray("servers");
            for (int i = 0; i < servers.length(); i++) {
                JSONObject server = servers.getJSONObject(i);
                VpnServerRegion vpnServerRegion = new VpnServerRegion(server.getString("continent"),
                        server.getString("name"), server.getString("name-pretty"));
                vpnServerRegions.add(vpnServerRegion);
            }
        } catch (JSONException e) {
            Log.e("BraveVPN", "getServerLocations JSONException error " + e);
        }
    }
}
