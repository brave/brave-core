/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.activities;

import static com.android.billingclient.api.BillingClient.SkuType.SUBS;

import android.content.Intent;
import android.util.Pair;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.core.content.ContextCompat;

import com.android.billingclient.api.Purchase;
import com.wireguard.android.backend.GoBackend;
import com.wireguard.crypto.KeyPair;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.models.BraveVpnPrefModel;
import org.chromium.chrome.browser.vpn.models.BraveVpnWireguardProfileCredentials;
import org.chromium.chrome.browser.vpn.utils.BraveVpnApiResponseUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.utils.InAppPurchaseWrapper;
import org.chromium.chrome.browser.vpn.wireguard.WireguardConfigUtils;
import org.chromium.chrome.browser.vpn.wireguard.WireguardService;
import org.chromium.ui.widget.Toast;

import java.util.List;
import java.util.TimeZone;

public abstract class BraveVpnParentActivity
        extends AsyncInitializationActivity implements BraveVpnObserver {
    public boolean mIsVerification;
    private BraveVpnPrefModel mBraveVpnPrefModel;

    abstract void showRestoreMenu(boolean shouldShowRestore);
    // abstract void showProgress();
    // abstract void hideProgress();
    abstract void updateProfileView();

    ActivityResultLauncher<Intent> intentActivityResultLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(), result -> {
                if (result.getResultCode() == RESULT_OK) {
                    BraveVpnProfileUtils.getInstance().startVpn(BraveVpnParentActivity.this);
                    BraveVpnUtils.showVpnConfirmDialog(this);
                } else if (result.getResultCode() == RESULT_CANCELED) {
                    if (BraveVpnProfileUtils.getInstance().isVPNRunning(this)) {
                        BraveVpnUtils.showVpnAlwaysOnErrorDialog(this);
                    } else {
                        updateProfileView();
                    }
                    Toast.makeText(this, "Permission cancelled", Toast.LENGTH_SHORT).show();
                }
            });

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            InAppPurchaseWrapper.getInstance().startBillingServiceConnection(
                    BraveVpnParentActivity.this);
        }
    }

    protected void verifySubscription() {
        // showProgress();
        // BraveVpnUtils.showProgressDialog(
        //         BraveVpnParentActivity.this,
        //         getResources().getString(R.string.vpn_connect_text));
        mBraveVpnPrefModel = new BraveVpnPrefModel();
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        Log.e("BraveVPN", "ParentActivity verifySubscription : 1");
        if (purchases != null && purchases.size() == 1) {
            Log.e("BraveVPN", "ParentActivity verifySubscription : 2");
            Purchase purchase = purchases.get(0);
            mBraveVpnPrefModel.setPurchaseToken(purchase.getPurchaseToken());
            Log.e("BraveVPN",
                    "ParentActivity verifySubscription : purchase token :"
                            + purchase.getPurchaseToken());
            mBraveVpnPrefModel.setProductId(purchase.getSkus().get(0).toString());
            BraveVpnNativeWorker.getInstance().verifyPurchaseToken(
                    mBraveVpnPrefModel.getPurchaseToken(), mBraveVpnPrefModel.getProductId(),
                    BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, getPackageName());
        } else {
            Log.e("BraveVPN", "ParentActivity verifySubscription : 3");
            if (!mIsVerification) {
                Log.e("BraveVPN", "ParentActivity verifySubscription : 4");
                BraveVpnApiResponseUtils.queryPurchaseFailed(BraveVpnParentActivity.this);
            } else {
                // hideProgress();
                Log.e("BraveVPN", "ParentActivity verifySubscription : 5");
                BraveVpnUtils.dismissProgressDialog();
            }
        }
    }

    @Override
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        if (isSuccess && mBraveVpnPrefModel != null) {
            Log.e("BraveVPN", "ParentActivity onVerifyPurchaseToken : 1");
            Long purchaseExpiry = BraveVpnUtils.getPurchaseExpiryDate(jsonResponse);
            if (purchaseExpiry > 0 && purchaseExpiry >= System.currentTimeMillis()) {
                Log.e("BraveVPN", "ParentActivity onVerifyPurchaseToken : 2");
                BraveVpnPrefUtils.setPurchaseToken(mBraveVpnPrefModel.getPurchaseToken());
                BraveVpnPrefUtils.setProductId(mBraveVpnPrefModel.getProductId());
                BraveVpnPrefUtils.setPurchaseExpiry(purchaseExpiry);
                BraveVpnPrefUtils.setSubscriptionPurchase(true);
                if (!mIsVerification || BraveVpnPrefUtils.isResetConfiguration()) {
                    Log.e("BraveVPN", "ParentActivity onVerifyPurchaseToken : 3");
                    BraveVpnNativeWorker.getInstance().getSubscriberCredential(
                            BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT,
                            mBraveVpnPrefModel.getProductId(), BraveVpnUtils.IAP_ANDROID_PARAM_TEXT,
                            mBraveVpnPrefModel.getPurchaseToken(), getPackageName());
                } else {
                    Log.e("BraveVPN", "ParentActivity onVerifyPurchaseToken : 4");
                    mIsVerification = false;
                    showRestoreMenu(true);
                    Toast.makeText(BraveVpnParentActivity.this, R.string.already_subscribed,
                                 Toast.LENGTH_SHORT)
                            .show();
                    // hideProgress();
                    BraveVpnUtils.dismissProgressDialog();
                }
            } else {
                Log.e("BraveVPN", "ParentActivity onVerifyPurchaseToken : 5");
                BraveVpnApiResponseUtils.queryPurchaseFailed(BraveVpnParentActivity.this);
                if (mIsVerification) {
                    mIsVerification = false;
                    showRestoreMenu(false);
                    // hideProgress();
                    BraveVpnUtils.dismissProgressDialog();
                } else {
                    BraveVpnUtils.openBraveVpnPlansActivity(BraveVpnParentActivity.this);
                }
            }
        } else {
            Log.e("BraveVPN", "ParentActivity onVerifyPurchaseToken : 6");
            // hideProgress();
            BraveVpnUtils.dismissProgressDialog();
        }
    };

    @Override
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        Log.e("BraveVPN", "ParentActivity onGetSubscriberCredential : " + subscriberCredential);
        mBraveVpnPrefModel.setSubscriberCredential(subscriberCredential);
        BraveVpnApiResponseUtils.handleOnGetSubscriberCredential(
                BraveVpnParentActivity.this, isSuccess);
    };

    @Override
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        BraveVpnApiResponseUtils.handleOnGetTimezonesForRegions(
                BraveVpnParentActivity.this, mBraveVpnPrefModel, jsonTimezones, isSuccess);
    }

    @Override
    public void onGetHostnamesForRegion(String jsonHostNames, boolean isSuccess) {
        KeyPair keyPair = new KeyPair();
        mBraveVpnPrefModel.setClientPrivateKey(keyPair.getPrivateKey().toBase64());
        mBraveVpnPrefModel.setClientPublicKey(keyPair.getPublicKey().toBase64());
        Pair<String, String> host = BraveVpnApiResponseUtils.handleOnGetHostnamesForRegion(
                BraveVpnParentActivity.this, mBraveVpnPrefModel, jsonHostNames, isSuccess);
        mBraveVpnPrefModel.setHostname(host.first);
        mBraveVpnPrefModel.setHostnameDisplay(host.second);
    }

    @Override
    public void onGetProfileCredentials(String jsonProfileCredentials, boolean isSuccess) {
        // BraveVpnApiResponseUtils.handleOnGetProfileCredentials(
        //         BraveVpnParentActivity.this, mBraveVpnPrefModel, jsonProfileCredentials,
        //         isSuccess);
        // Intent intent = GoBackend.VpnService.prepare(this);
        // if (intent != null) {
        //     intentActivityResultLauncher.launch(intent);
        //     return;
        // }
        // ContextCompat.startForegroundService(this, new Intent(this, WireguardService.class));
    }

    @Override
    public void onGetWireguardProfileCredentials(
            String jsonWireguardProfileCredentials, boolean isSuccess) {
        // BraveVpnApiResponseUtils.handleOnGetWireguardProfileCredential(BraveVpnParentActivity.this,
        // mBraveVpnPrefModel, jsonWireguardProfileCredentials, isSuccess);

        Log.e("BraveVPN", "onGetWireguardProfileCredentials : 0");
        if (isSuccess && mBraveVpnPrefModel != null) {
            Log.e("BraveVPN", "onGetWireguardProfileCredentials : 1");
            BraveVpnWireguardProfileCredentials braveVpnWireguardProfileCredentials =
                    BraveVpnUtils.getWireguardProfileCredentials(jsonWireguardProfileCredentials);
            if (BraveVpnProfileUtils.getInstance().isBraveVPNConnected(this)) {
                BraveVpnProfileUtils.getInstance().stopVpn(this);
            }
            Log.e("BraveVPN", "onGetWireguardProfileCredentials : 2");
            mBraveVpnPrefModel.setClientId(braveVpnWireguardProfileCredentials.getClientId());
            mBraveVpnPrefModel.setApiAuthToken(
                    braveVpnWireguardProfileCredentials.getApiAuthToken());
            BraveVpnPrefUtils.setPrefModel(mBraveVpnPrefModel);

            try {
                if (!WireguardConfigUtils.isConfigExist(getApplicationContext())) {
                    WireguardConfigUtils.createConfig(getApplicationContext(),
                            braveVpnWireguardProfileCredentials.getMappedIpv4Address(),
                            mBraveVpnPrefModel.getHostname(),
                            mBraveVpnPrefModel.getClientPrivateKey(),
                            braveVpnWireguardProfileCredentials.getServerPublicKey());
                }

                Intent intent = GoBackend.VpnService.prepare(this);
                if (intent != null) {
                    intentActivityResultLauncher.launch(intent);
                    return;
                }
                BraveVpnProfileUtils.getInstance().startVpn(BraveVpnParentActivity.this);
                BraveVpnUtils.dismissProgressDialog();
                finish();
            } catch (Exception e) {
                Log.e("Tunnel", e.getMessage());
            }
        } else {
            Toast.makeText(this, R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG).show();
            Log.e("BraveVPN", "onGetWireguardProfileCredentials : failed");
            BraveVpnUtils.dismissProgressDialog();
        }

        // if (isSuccess) {
        //     Intent intent = GoBackend.VpnService.prepare(this);
        //     if (intent != null) {
        //         intentActivityResultLauncher.launch(intent);
        //         return;
        //     }
        //     BraveVpnProfileUtils.getInstance().startVpn(BraveVpnParentActivity.this);
        // }
    }
}
