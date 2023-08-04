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

import androidx.annotation.NonNull;

import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesResponseListener;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.models.BraveVpnPrefModel;
import org.chromium.ui.widget.Toast;

import java.util.List;
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
        Toast.makeText(activity, R.string.purchase_token_verification_failed, Toast.LENGTH_LONG)
                .show();
        BraveVpnUtils.dismissProgressDialog();
    }

    public static void handleOnGetSubscriberCredential(Activity activity, boolean isSuccess) {
        if (isSuccess) {
            if (!BraveVpnNativeWorker.getInstance().isPurchasedUser()) {
                InAppPurchaseWrapper.getInstance().queryPurchases(new PurchasesResponseListener() {
                    @Override
                    public void onQueryPurchasesResponse(@NonNull BillingResult billingResult,
                            @NonNull List<Purchase> purchases) {
                        InAppPurchaseWrapper.getInstance().processPurchases(activity, purchases);
                    }
                });
            }
            BraveVpnNativeWorker.getInstance().getTimezonesForRegions();
        } else {
            Toast.makeText(activity, R.string.vpn_profile_creation_failed, Toast.LENGTH_SHORT)
                    .show();
            BraveVpnUtils.dismissProgressDialog();
        }
    }

    public static void handleOnGetTimezonesForRegions(Activity activity,
            BraveVpnPrefModel braveVpnPrefModel, String jsonTimezones, boolean isSuccess) {
        if (isSuccess) {
            String region = BraveVpnUtils.getRegionForTimeZone(
                    jsonTimezones, TimeZone.getDefault().getID());
            if (TextUtils.isEmpty(region)) {
                Toast.makeText(activity,
                             String.format(activity.getResources().getString(
                                                   R.string.couldnt_get_matching_timezone),
                                     TimeZone.getDefault().getID()),
                             Toast.LENGTH_LONG)
                        .show();
                return;
            }
            if (!TextUtils.isEmpty(BraveVpnUtils.selectedServerRegion)
                    && BraveVpnUtils.selectedServerRegion != null) {
                region = BraveVpnUtils.selectedServerRegion.equals(
                                 BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC)
                        ? region
                        : BraveVpnUtils.selectedServerRegion;
                BraveVpnUtils.selectedServerRegion = null;
            } else {
                String serverRegion = BraveVpnPrefUtils.getServerRegion();
                region = serverRegion.equals(BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC)
                        ? region
                        : serverRegion;
            }

            BraveVpnNativeWorker.getInstance().getHostnamesForRegion(region);
            braveVpnPrefModel.setServerRegion(region);
        } else {
            Toast.makeText(activity, R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
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
            Toast.makeText(activity, R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            BraveVpnUtils.dismissProgressDialog();
        }
        return host;
    }
}
