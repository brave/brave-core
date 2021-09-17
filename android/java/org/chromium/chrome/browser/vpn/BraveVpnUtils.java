/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.app.Activity;
import android.app.NotificationManager;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.text.TextUtils;
import android.util.Pair;

import androidx.core.app.NotificationCompat;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.vpn.BraveVpnPlansActivity;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnProfileActivity;
import org.chromium.chrome.browser.vpn.BraveVpnServerRegion;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class BraveVpnUtils {
    public static final String SUBSCRIPTION_PARAM_TEXT = "subscription";
    public static final String IAP_ANDROID_PARAM_TEXT = "iap-android";

    public static final int BRAVE_VPN_NOTIFICATION_ID = 36;

    public static boolean mIsServerLocationChanged;
    private static ProgressDialog mProgressDialog;

    public static boolean isBraveVpnFeatureEnable() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R
                && BraveVpnPrefUtils.isBraveVpnFeatureEnabled()) {
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

    public static void openBraveVpnSupportActivity(Context context) {
        Intent braveVpnSupportIntent = new Intent(context, BraveVpnSupportActivity.class);
        braveVpnSupportIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        context.startActivity(braveVpnSupportIntent);
    }

    public static void showProgressDialog(Context context) {
        mProgressDialog = ProgressDialog.show(
                context, "", context.getResources().getString(R.string.vpn_loading_text), true);
    }

    public static void dismissProgressDialog() {
        if (mProgressDialog != null && mProgressDialog.isShowing()) {
            mProgressDialog.dismiss();
        }
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
            Log.e("BraveVPN", "BraveVpnUtils -> getRegionForTimeZone JSONException error " + e);
        }
        return "";
    }

    public static String getHostnameForRegion(String jsonHostnames) {
        jsonHostnames = "{\"hostnames\":" + jsonHostnames + "}";
        try {
            JSONObject result = new JSONObject(jsonHostnames);
            JSONArray hostnames = result.getJSONArray("hostnames");
            ArrayList<JSONObject> hosts = new ArrayList<JSONObject>();
            for (int i = 0; i < hostnames.length(); i++) {
                JSONObject hostname = hostnames.getJSONObject(i);
                if (hostname.getInt("capacity-score") == 0
                        || hostname.getInt("capacity-score") == 1) {
                    hosts.add(hostname);
                }
            }

            JSONObject hostname;
            if (hosts.size() < 2) {
                hostname = hostnames.getJSONObject(new Random().nextInt(hostnames.length()));
            } else {
                hostname = hosts.get(new Random().nextInt(hosts.size()));
            }
            return hostname.getString("hostname");
        } catch (JSONException e) {
            Log.e("BraveVPN", "BraveVpnUtils -> getHostnameForRegion JSONException error " + e);
        }
        return "";
    }

    public static Pair<String, String> getProfileCredentials(String jsonProfileCredentials) {
        try {
            JSONObject profileCredentials = new JSONObject(jsonProfileCredentials);
            return new Pair<>(profileCredentials.getString("eap-username"),
                    profileCredentials.getString("eap-password"));
        } catch (JSONException e) {
            Log.e("BraveVPN", "BraveVpnUtils -> getProfileCredentials JSONException error " + e);
        }
        return null;
    }

    public static Long getPurchaseExpiryDate(String json) {
        try {
            JSONObject purchase = new JSONObject(json);
            String expiryTimeInString = purchase.getString("expiryTimeMillis");
            return Long.parseLong(expiryTimeInString);
        } catch (JSONException | NumberFormatException e) {
            Log.e("BraveVPN",
                    "BraveVpnUtils -> getPurchaseExpiryDate JSONException | NumberFormatException error "
                            + e);
        }
        return 0L;
    }

    public static List<BraveVpnServerRegion> getServerLocations(String jsonServerLocations) {
        List<BraveVpnServerRegion> vpnServerRegions = new ArrayList<>();
        if (TextUtils.isEmpty(jsonServerLocations)) {
            return vpnServerRegions;
        }
        jsonServerLocations = "{\"servers\":" + jsonServerLocations + "}";
        try {
            JSONObject result = new JSONObject(jsonServerLocations);
            JSONArray servers = result.getJSONArray("servers");
            for (int i = 0; i < servers.length(); i++) {
                JSONObject server = servers.getJSONObject(i);
                BraveVpnServerRegion vpnServerRegion =
                        new BraveVpnServerRegion(server.getString("continent"),
                                server.getString("name"), server.getString("name-pretty"));
                vpnServerRegions.add(vpnServerRegion);
            }
        } catch (JSONException e) {
            Log.e("BraveVPN", "BraveVpnUtils -> getServerLocations JSONException error " + e);
        }
        return vpnServerRegions;
    }

    public static void showBraveVpnNotification(Context context) {
        NotificationCompat.Builder notificationBuilder =
                new NotificationCompat.Builder(context, BraveActivity.CHANNEL_ID);

        notificationBuilder.setSmallIcon(R.drawable.ic_chrome)
                .setAutoCancel(false)
                .setContentTitle(context.getResources().getString(R.string.brave_firewall_vpn))
                .setContentText(
                        context.getResources().getString(R.string.brave_vpn_notification_message))
                .setStyle(new NotificationCompat.BigTextStyle().bigText(
                        context.getResources().getString(R.string.brave_vpn_notification_message)))
                .setPriority(NotificationCompat.PRIORITY_DEFAULT);

        NotificationManager notificationManager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.notify(BRAVE_VPN_NOTIFICATION_ID, notificationBuilder.build());
    }

    public static void cancelBraveVpnNotification(Context context) {
        NotificationManager notificationManager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(BRAVE_VPN_NOTIFICATION_ID);
    }
}
