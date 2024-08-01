/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.utils;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;
import android.util.Pair;

import androidx.fragment.app.FragmentActivity;

import com.wireguard.config.Config;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.activities.BraveVpnProfileActivity;
import org.chromium.chrome.browser.vpn.activities.BraveVpnSupportActivity;
import org.chromium.chrome.browser.vpn.activities.VpnAlwaysOnActivity;
import org.chromium.chrome.browser.vpn.activities.VpnPaywallActivity;
import org.chromium.chrome.browser.vpn.activities.VpnServerActivity;
import org.chromium.chrome.browser.vpn.activities.VpnServerSelectionActivity;
import org.chromium.chrome.browser.vpn.fragments.BraveVpnAlwaysOnErrorDialogFragment;
import org.chromium.chrome.browser.vpn.fragments.BraveVpnConfirmDialogFragment;
import org.chromium.chrome.browser.vpn.models.BraveVpnServerRegion;
import org.chromium.chrome.browser.vpn.models.BraveVpnWireguardProfileCredentials;
import org.chromium.chrome.browser.vpn.split_tunnel.SplitTunnelActivity;
import org.chromium.chrome.browser.vpn.wireguard.WireguardConfigUtils;
import org.chromium.gms.ChromiumPlayServicesAvailability;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class BraveVpnUtils {
    private static final String TAG = "BraveVPN";
    public static final String SUBSCRIPTION_PARAM_TEXT = "subscription";
    public static final String IAP_ANDROID_PARAM_TEXT = "iap-android";
    public static final String VERIFY_CREDENTIALS_FAILED = "verify_credentials_failed";
    public static final String DESKTOP_CREDENTIAL = "desktop_credential";

    public static boolean mUpdateProfileAfterSplitTunnel;
    public static BraveVpnServerRegion selectedServerRegion;
    private static ProgressDialog sProgressDialog;

    public static String IS_KILL_SWITCH = "is_kill_switch";

    public static void openBraveVpnPlansActivity(Activity activity) {
        if (activity == null) {
            return;
        }
        Intent braveVpnPlanIntent = new Intent(activity, VpnPaywallActivity.class);
        braveVpnPlanIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        braveVpnPlanIntent.setAction(Intent.ACTION_VIEW);
        activity.startActivity(braveVpnPlanIntent);
    }

    public static void openBraveVpnProfileActivity(Activity activity) {
        if (activity == null) {
            return;
        }
        Intent braveVpnProfileIntent = new Intent(activity, BraveVpnProfileActivity.class);
        braveVpnProfileIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        braveVpnProfileIntent.setAction(Intent.ACTION_VIEW);
        activity.startActivity(braveVpnProfileIntent);
    }

    public static void openBraveVpnSupportActivity(Activity activity) {
        if (activity == null) {
            return;
        }
        Intent braveVpnSupportIntent = new Intent(activity, BraveVpnSupportActivity.class);
        braveVpnSupportIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        braveVpnSupportIntent.setAction(Intent.ACTION_VIEW);
        activity.startActivity(braveVpnSupportIntent);
    }

    public static void openSplitTunnelActivity(Activity activity) {
        if (activity == null) {
            return;
        }
        Intent braveVpnSupportIntent = new Intent(activity, SplitTunnelActivity.class);
        braveVpnSupportIntent.setAction(Intent.ACTION_VIEW);
        activity.startActivity(braveVpnSupportIntent);
    }

    public static void openAlwaysOnActivity(Activity activity) {
        if (activity == null) {
            return;
        }
        Intent vpnAlwaysOnActivityIntent = new Intent(activity, VpnAlwaysOnActivity.class);
        activity.startActivity(vpnAlwaysOnActivityIntent);
    }

    public static void openVpnSettings(Activity activity) {
        if (activity == null) {
            return;
        }
        Intent vpnSettingsIntent = new Intent("android.net.vpn.SETTINGS");
        vpnSettingsIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        activity.startActivity(vpnSettingsIntent);
    }

    public static void openVpnServerSelectionActivity(Activity activity) {
        if (activity == null) {
            return;
        }
        Intent vpnServerSelectionIntent = new Intent(activity, VpnServerSelectionActivity.class);
        activity.startActivity(vpnServerSelectionIntent);
    }

    public static void openVpnServerActivity(Activity activity) {
        if (activity == null) {
            return;
        }
        Intent vpnServerIntent = new Intent(activity, VpnServerActivity.class);
        activity.startActivity(vpnServerIntent);
    }

    public static void showProgressDialog(Activity activity, String message) {
        sProgressDialog = ProgressDialog.show(activity, "", message, true);
    }

    public static void dismissProgressDialog() {
        if (sProgressDialog != null && sProgressDialog.isShowing()) {
            sProgressDialog.dismiss();
        }
    }

    public static BraveVpnServerRegion getServerRegionForTimeZone(
            String jsonTimezones, String currentTimezone) {
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
                        BraveVpnServerRegion braveVpnServerRegion =
                                new BraveVpnServerRegion(
                                        region.getString("continent"),
                                                region.getString("country-iso-code"),
                                        region.getString("name"), region.getString("name-pretty"));
                        return braveVpnServerRegion;
                    }
                }
            }
        } catch (JSONException e) {
            Log.e(TAG, "BraveVpnUtils -> getRegionForTimeZone JSONException error " + e);
        }
        return null;
    }

    public static Pair<String, String> getHostnameForRegion(String jsonHostnames) {
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
            return new Pair<>(hostname.getString("hostname"), hostname.getString("display-name"));
        } catch (JSONException e) {
            Log.e(TAG, "BraveVpnUtils -> getHostnameForRegion JSONException error " + e);
        }
        return new Pair<String, String>("", "");
    }

    public static BraveVpnWireguardProfileCredentials getWireguardProfileCredentials(
            String jsonWireguardProfileCredentials) {
        try {
            JSONObject wireguardProfileCredentials =
                    new JSONObject(jsonWireguardProfileCredentials);
            BraveVpnWireguardProfileCredentials braveVpnWireguardProfileCredentials =
                    new BraveVpnWireguardProfileCredentials(
                            wireguardProfileCredentials.getString("api-auth-token"),
                            wireguardProfileCredentials.getString("client-id"),
                            wireguardProfileCredentials.getString("mapped-ipv4-address"),
                            wireguardProfileCredentials.getString("mapped-ipv6-address"),
                            wireguardProfileCredentials.getString("server-public-key"));
            return braveVpnWireguardProfileCredentials;
        } catch (JSONException e) {
            Log.e(TAG, "BraveVpnUtils -> getWireguardProfileCredentials JSONException error " + e);
        }
        return null;
    }

    public static Long getPurchaseExpiryDate(String json) {
        try {
            JSONObject purchase = new JSONObject(json);
            String expiryTimeInString = purchase.getString("expiryTimeMillis");
            return Long.parseLong(expiryTimeInString);
        } catch (JSONException | NumberFormatException e) {
            Log.e(
                    TAG,
                    "BraveVpnUtils -> getPurchaseExpiryDate JSONException | NumberFormatException"
                            + " error "
                            + e);
        }
        return 0L;
    }

    public static int getPaymentState(String json) {
        try {
            JSONObject purchase = new JSONObject(json);
            int paymentState = purchase.getInt("paymentState");
            return paymentState;
        } catch (JSONException e) {
            Log.e(TAG, "BraveVpnUtils -> getPaymentState JSONException error " + e);
        }
        return 0;
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
                        new BraveVpnServerRegion(
                                server.getString("continent"), server.getString("country-iso-code"),
                                server.getString("name"), server.getString("name-pretty"));
                vpnServerRegions.add(vpnServerRegion);
            }
        } catch (JSONException e) {
            Log.e(TAG, "BraveVpnUtils -> getServerLocations JSONException error " + e);
        }
        return vpnServerRegions;
    }

    public static void resetProfileConfiguration(Activity activity) {
        if (BraveVpnProfileUtils.getInstance().isBraveVPNConnected(activity)) {
            BraveVpnProfileUtils.getInstance().stopVpn(activity);
        }
        try {
            WireguardConfigUtils.deleteConfig(activity);
        } catch (Exception ex) {
            Log.e(TAG, "resetProfileConfiguration : " + ex.getMessage());
        }
        BraveVpnPrefUtils.setResetConfiguration(true);
        dismissProgressDialog();
    }

    public static void updateProfileConfiguration(Activity activity) {
        try {
            Config existingConfig = WireguardConfigUtils.loadConfig(activity);
            WireguardConfigUtils.deleteConfig(activity);
            WireguardConfigUtils.createConfig(activity, existingConfig);
        } catch (Exception ex) {
            Log.e(TAG, "updateProfileConfiguration : " + ex.getMessage());
        }
        if (BraveVpnProfileUtils.getInstance().isBraveVPNConnected(activity)) {
            BraveVpnProfileUtils.getInstance().stopVpn(activity);
            BraveVpnProfileUtils.getInstance().startVpn(activity);
        }
        dismissProgressDialog();
    }

    public static void showVpnAlwaysOnErrorDialog(Activity activity) {
        BraveVpnAlwaysOnErrorDialogFragment mBraveVpnAlwaysOnErrorDialogFragment =
                new BraveVpnAlwaysOnErrorDialogFragment();
        mBraveVpnAlwaysOnErrorDialogFragment.show(
                ((FragmentActivity) activity).getSupportFragmentManager(),
                "BraveVpnAlwaysOnErrorDialogFragment");
    }

    public static void showVpnConfirmDialog(Activity activity) {
        BraveVpnConfirmDialogFragment braveVpnConfirmDialogFragment =
                new BraveVpnConfirmDialogFragment();
        braveVpnConfirmDialogFragment.show(
                ((FragmentActivity) activity).getSupportFragmentManager(),
                "BraveVpnConfirmDialogFragment");
    }

    public static void reportBackgroundUsageP3A() {
        // Will report previous/current session timestamps...
        BraveVpnNativeWorker.getInstance().reportBackgroundP3A(
                BraveVpnPrefUtils.getSessionStartTimeMs(), BraveVpnPrefUtils.getSessionEndTimeMs());
        // ...and then reset the timestamps so we don't report the same session again.
        BraveVpnPrefUtils.setSessionStartTimeMs(-1);
        BraveVpnPrefUtils.setSessionEndTimeMs(-1);
    }

    public static void showToast(String message) {
        Context context = ContextUtils.getApplicationContext();
        Toast.makeText(context, message, Toast.LENGTH_SHORT).show();
    }

    private static boolean isRegionSupported() {
        BraveRewardsNativeWorker braveRewardsNativeWorker = BraveRewardsNativeWorker.getInstance();
        return (braveRewardsNativeWorker != null && braveRewardsNativeWorker.isSupported());
    }

    public static boolean isVpnFeatureSupported(Context context) {
        return isRegionSupported()
                && ChromiumPlayServicesAvailability.isGooglePlayServicesAvailable(context);
    }

    public static String countryCodeToEmoji(String countryCode) {
        int firstLetter = Character.codePointAt(countryCode, 0) - 0x41 + 0x1F1E6;
        int secondLetter = Character.codePointAt(countryCode, 1) - 0x41 + 0x1F1E6;
        return new String(Character.toChars(firstLetter))
                + new String(Character.toChars(secondLetter));
    }
}
