/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.utils;

import android.app.Activity;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.models.BraveVpnPrefModel;
import org.chromium.chrome.browser.vpn.models.BraveVpnProfileCredentials;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.utils.InAppPurchaseWrapper;
import org.chromium.ui.widget.Toast;

import java.util.List;
import java.util.TimeZone;

public class BraveVpnApiResponseUtils {
    public static void queryPurchaseFailed(Activity activity) {
        Log.e("BraveVPN", "BraveVpnApiResponseUtils 4");
        BraveVpnPrefUtils.setPurchaseToken("");
        BraveVpnPrefUtils.setProductId("");
        BraveVpnPrefUtils.setPurchaseExpiry(0L);
        BraveVpnPrefUtils.setSubscriptionPurchase(false);
        if (BraveVpnProfileUtils.getInstance().isVPNConnected(activity)) {
            BraveVpnProfileUtils.getInstance().stopVpn(activity);
        }
        BraveVpnProfileUtils.getInstance().deleteVpnProfile(activity);
        Toast.makeText(activity, R.string.purchase_token_verification_failed, Toast.LENGTH_LONG)
                .show();
        BraveVpnUtils.dismissProgressDialog();
        BraveVpnUtils.openBraveVpnPlansActivity(activity);
    }

    public static void handleOnGetSubscriberCredential(Activity activity, boolean isSuccess) {
        if (isSuccess) {
            InAppPurchaseWrapper.getInstance().processPurchases(
                    activity, InAppPurchaseWrapper.getInstance().queryPurchases());
            BraveVpnNativeWorker.getInstance().getTimezonesForRegions();
        } else {
            Toast.makeText(activity, R.string.vpn_profile_creation_failed, Toast.LENGTH_SHORT)
                    .show();
            Log.e("BraveVPN", "handleOnGetSubscriberCredential : failed");
            BraveVpnUtils.dismissProgressDialog();
        }
    }

    public static void handleOnGetTimezonesForRegions(
            Activity activity, String jsonTimezones, boolean isSuccess) {
        if (isSuccess) {
            String region = BraveVpnUtils.getRegionForTimeZone(
                    jsonTimezones, TimeZone.getDefault().getID());
            String serverRegion = BraveVpnPrefUtils.getServerRegion();
            BraveVpnNativeWorker.getInstance().getHostnamesForRegion(
                    serverRegion.equals(BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC) ? region
                                                                                    : serverRegion);
        } else {
            Toast.makeText(activity, R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "handleOnGetTimezonesForRegions : failed");
            BraveVpnUtils.dismissProgressDialog();
        }
    }

    public static String handleOnGetHostnamesForRegion(Activity activity,
            BraveVpnPrefModel braveVpnPrefModel, String jsonHostNames, boolean isSuccess) {
        String hostname = "";
        if (isSuccess && braveVpnPrefModel != null) {
            hostname = BraveVpnUtils.getHostnameForRegion(jsonHostNames);
            BraveVpnNativeWorker.getInstance().getProfileCredentials(
                    braveVpnPrefModel.getSubscriberCredential(), hostname);
        } else {
            Toast.makeText(activity, R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "handleOnGetHostnamesForRegion : failed");
            BraveVpnUtils.dismissProgressDialog();
        }
        return hostname;
    }

    public static void handleOnGetProfileCredentials(Activity activity,
            BraveVpnPrefModel braveVpnPrefModel, String jsonProfileCredentials, boolean isSuccess) {
        if (isSuccess && braveVpnPrefModel != null) {
            BraveVpnProfileCredentials braveVpnProfileCredentials =
                    BraveVpnUtils.getProfileCredentials(jsonProfileCredentials);
            if (BraveVpnProfileUtils.getInstance().isVPNConnected(activity)) {
                BraveVpnProfileUtils.getInstance().stopVpn(activity);
            }
            BraveVpnPrefUtils.setPrefModel(braveVpnPrefModel);
            try {
                BraveVpnProfileUtils.getInstance().createVpnProfile(activity,
                        braveVpnPrefModel.getHostname(), braveVpnProfileCredentials.getUsername(),
                        braveVpnProfileCredentials.getPassword());
            } catch (Exception securityException) {
                BraveVpnProfileUtils.getInstance().startVpn(activity);
                // activity.finish();
            }
        } else {
            Toast.makeText(activity, R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "handleOnGetProfileCredentials : failed");
            BraveVpnUtils.dismissProgressDialog();
        }
    }
}
