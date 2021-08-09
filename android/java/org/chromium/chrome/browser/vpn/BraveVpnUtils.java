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

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.vpn.BraveVpnPlansActivity;
import org.chromium.chrome.browser.vpn.BraveVpnProfileActivity;

public class BraveVpnUtils {
    public static final int MONTHLY_SUBSCRIPTION = 1;
    public static final int YEARLY_SUBSCRIPTION = 2;

    public static final int BRAVE_VPN_PROFILE_REQUEST_CODE = 36;

    private static final String PREF_BRAVE_VPN_CALLOUT = "brave_vpn_callout";

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

    public static boolean isVPNConnected(Context context) {
        ConnectivityManager connectivityManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivityManager != null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                NetworkCapabilities capabilities = connectivityManager.getNetworkCapabilities(
                        connectivityManager.getActiveNetwork());
                return capabilities.hasTransport(NetworkCapabilities.TRANSPORT_VPN);
            } else {
                NetworkInfo activeNetwork = connectivityManager.getActiveNetworkInfo();
                return activeNetwork.getType() == ConnectivityManager.TYPE_VPN;
            }
        }
        return false;
    }

    public static Ikev2VpnProfile getVpnProfile(Context context) {
        Ikev2VpnProfile.Builder builder = new Ikev2VpnProfile.Builder("Server Url", "identity");
        return builder.setAuthUsernamePassword("username", "password", null).build();
    }

    public static boolean isBraveVpnFeatureEnable() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return true;
        }
        return false;
    }

    public static void openBraveVpnPlansActivity(Context context) {
        Intent braveVpnPlanIntent = new Intent(context, BraveVpnPlansActivity.class);
        braveVpnPlanIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        context.startActivity(braveVpnPlanIntent);
    }

    public static void openBraveVpnProfileActivity(Context context) {
        Intent braveVpnProfileIntent = new Intent(context, BraveVpnProfileActivity.class);
        braveVpnProfileIntent.setFlags(
                Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        context.startActivity(braveVpnProfileIntent);
    }

    public static void startStopVpn(Activity activity) {
        if (isBraveVpnFeatureEnable()) {
            VpnManager vpnManager =
                    (VpnManager) activity.getSystemService(Context.VPN_MANAGEMENT_SERVICE);
            if (!isVPNConnected(activity) && vpnManager != null) {
                try {
                    vpnManager.startProvisionedVpnProfile();
                } catch (SecurityException securityException) {
                    Ikev2VpnProfile ikev2VpnProfile = BraveVpnUtils.getVpnProfile(activity);
                    Intent intent = vpnManager.provisionVpnProfile(ikev2VpnProfile);
                    activity.startActivityForResult(intent, BRAVE_VPN_PROFILE_REQUEST_CODE);
                }
            } else {
                vpnManager.stopProvisionedVpnProfile();
            }
        }
    }
}
