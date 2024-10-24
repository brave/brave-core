/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.utils;

import android.app.Activity;
import android.text.TextUtils;
import android.util.Pair;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.chromium.brave_vpn.mojom.BraveVpnConstants;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.PurchaseModel;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.models.BraveVpnPrefModel;
import org.chromium.chrome.browser.vpn.models.BraveVpnServerRegion;

import java.util.TimeZone;

public class BraveVpnApiResponseUtils {
    public static void queryPurchaseFailed(Activity activity) {
        BraveVpnPrefUtils.setProductId("");
        BraveVpnPrefUtils.setPurchaseExpiry(0L);
        BraveVpnPrefUtils.setSubscriptionPurchase(false);
        BraveVpnPrefUtils.setPaymentState(0);
        if (BraveVpnProfileUtils.getInstance().isBraveVPNConnected(activity)) {
            BraveVpnProfileUtils.getInstance().stopVpn(activity);
        }
        BraveVpnUtils.showToast(
                activity.getResources().getString(R.string.purchase_token_verification_failed));
        BraveVpnUtils.dismissProgressDialog();
    }

    public static void handleOnGetSubscriberCredential(Activity activity, boolean isSuccess) {
        if (isSuccess) {
            if (!BraveVpnNativeWorker.getInstance().isPurchasedUser()) {
                MutableLiveData<PurchaseModel> _activePurchases = new MutableLiveData();
                LiveData<PurchaseModel> activePurchases = _activePurchases;
                InAppPurchaseWrapper.getInstance()
                        .queryPurchases(
                                _activePurchases, InAppPurchaseWrapper.SubscriptionProduct.VPN);
                LiveDataUtil.observeOnce(activePurchases, activePurchaseModel -> {
                    InAppPurchaseWrapper.getInstance().processPurchases(
                            activity, activePurchaseModel.getPurchase());
                });
            }
            BraveVpnNativeWorker.getInstance().getTimezonesForRegions();
        } else {
            BraveVpnUtils.showToast(
                    activity.getResources().getString(R.string.vpn_profile_creation_failed));
            BraveVpnUtils.dismissProgressDialog();
        }
    }

    public static void handleOnGetTimezonesForRegions(Activity activity,
            BraveVpnPrefModel braveVpnPrefModel, String jsonTimezones, boolean isSuccess) {
        if (isSuccess) {
            BraveVpnServerRegion braveVpnServerRegion =
                    BraveVpnUtils.getServerRegionForTimeZone(
                            jsonTimezones, TimeZone.getDefault().getID());
            String regionFromTimeZone = braveVpnServerRegion.getRegionName();
            if (TextUtils.isEmpty(regionFromTimeZone)) {
                BraveVpnUtils.showToast(
                        String.format(
                                activity.getResources()
                                        .getString(R.string.couldnt_get_matching_timezone),
                                TimeZone.getDefault().getID()));
                return;
            }
            String regionForHostName = regionFromTimeZone;
            String regionPrecision = braveVpnServerRegion.getRegionPrecision();

            // Determine the region for host name and precision
            if (BraveVpnUtils.selectedServerRegion != null
                    && !BraveVpnUtils.selectedServerRegion
                            .getRegionName()
                            .equals(BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC)) {

                regionForHostName = BraveVpnUtils.selectedServerRegion.getRegionName();
                braveVpnServerRegion = BraveVpnUtils.selectedServerRegion;
                regionPrecision = braveVpnServerRegion.getRegionPrecision();
            } else {
                String serverRegion = BraveVpnPrefUtils.getRegionName();
                if (serverRegion.equals(BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC)) {
                    regionPrecision = BraveVpnConstants.REGION_PRECISION_DEFAULT;
                } else {
                    regionForHostName = serverRegion;
                }
            }
            BraveVpnNativeWorker.getInstance()
                    .getHostnamesForRegion(regionForHostName, regionPrecision);
            braveVpnPrefModel.setServerRegion(braveVpnServerRegion);
        } else {
            BraveVpnUtils.showToast(
                    activity.getResources().getString(R.string.vpn_profile_creation_failed));
            BraveVpnUtils.dismissProgressDialog();
        }
    }

    public static Pair<String, String> handleOnGetHostnamesForRegion(Activity activity,
            BraveVpnPrefModel braveVpnPrefModel, String jsonHostNames, boolean isSuccess) {
        Pair<String, String> host = new Pair<String, String>("", "");
        if (isSuccess && braveVpnPrefModel != null) {
            host = BraveVpnUtils.getHostnameForRegion(jsonHostNames);
            BraveVpnNativeWorker.getInstance().getWireguardProfileCredentials(
                    braveVpnPrefModel.getSubscriberCredential(),
                    braveVpnPrefModel.getClientPublicKey(), host.first);
        } else {
            BraveVpnUtils.showToast(
                    activity.getResources().getString(R.string.vpn_profile_creation_failed));
            BraveVpnUtils.dismissProgressDialog();
        }
        return host;
    }
}
