/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.activities;

import android.content.Intent;
import android.util.Pair;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;

import com.android.billingclient.api.Purchase;
import com.wireguard.android.backend.GoBackend;
import com.wireguard.crypto.KeyPair;

import org.chromium.base.Log;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.base.supplier.OneshotSupplierImpl;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.profiles.Profile;
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
import org.chromium.ui.widget.Toast;

import java.util.List;

public abstract class BraveVpnParentActivity
        extends AsyncInitializationActivity implements BraveVpnObserver {
    private static final String TAG = "BraveVPN";
    public boolean mIsVerification;
    protected BraveVpnPrefModel mBraveVpnPrefModel;
    private final OneshotSupplierImpl<Profile> mProfileSupplier;

    abstract void showRestoreMenu(boolean shouldShowRestore);
    abstract void updateProfileView();

    // Pass @{code ActivityResultRegistry} reference explicitly to avoid crash
    // https://github.com/brave/brave-browser/issues/31882
    public BraveVpnParentActivity() {
        mProfileSupplier = new OneshotSupplierImpl<>();
    }

    ActivityResultLauncher<Intent> intentActivityResultLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(), getActivityResultRegistry(),
            result -> {
                BraveVpnUtils.dismissProgressDialog();
                if (result.getResultCode() == RESULT_OK) {
                    BraveVpnProfileUtils.getInstance().startVpn(BraveVpnParentActivity.this);
                    BraveVpnUtils.showVpnConfirmDialog(this);
                } else if (result.getResultCode() == RESULT_CANCELED) {
                    if (BraveVpnProfileUtils.getInstance().isVPNRunning(this)) {
                        BraveVpnUtils.showVpnAlwaysOnErrorDialog(this);
                    } else {
                        updateProfileView();
                    }
                    Toast.makeText(this,
                                 getResources().getString(R.string.permission_was_cancelled),
                                 Toast.LENGTH_SHORT)
                            .show();
                }
            });

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        mProfileSupplier.set(Profile.getLastUsedRegularProfile());
        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            InAppPurchaseWrapper.getInstance().startBillingServiceConnection(
                    BraveVpnParentActivity.this);
        }
    }

    protected void verifySubscription() {
        mBraveVpnPrefModel = new BraveVpnPrefModel();
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        if (purchases != null && purchases.size() == 1) {
            Purchase purchase = purchases.get(0);
            mBraveVpnPrefModel.setPurchaseToken(purchase.getPurchaseToken());
            mBraveVpnPrefModel.setProductId(purchase.getSkus().get(0).toString());
            BraveVpnNativeWorker.getInstance().verifyPurchaseToken(
                    mBraveVpnPrefModel.getPurchaseToken(), mBraveVpnPrefModel.getProductId(),
                    BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, getPackageName());
        } else {
            if (!mIsVerification) {
                BraveVpnApiResponseUtils.queryPurchaseFailed(BraveVpnParentActivity.this);
            } else {
                BraveVpnUtils.dismissProgressDialog();
            }
        }
    }

    @Override
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        if (isSuccess && mBraveVpnPrefModel != null) {
            Long purchaseExpiry = BraveVpnUtils.getPurchaseExpiryDate(jsonResponse);
            int paymentState = BraveVpnUtils.getPaymentState(jsonResponse);
            if (purchaseExpiry > 0 && purchaseExpiry >= System.currentTimeMillis()) {
                BraveVpnPrefUtils.setPurchaseToken(mBraveVpnPrefModel.getPurchaseToken());
                BraveVpnPrefUtils.setProductId(mBraveVpnPrefModel.getProductId());
                BraveVpnPrefUtils.setPurchaseExpiry(purchaseExpiry);
                BraveVpnPrefUtils.setSubscriptionPurchase(true);
                BraveVpnPrefUtils.setPaymentState(paymentState);
                if (!mIsVerification || BraveVpnPrefUtils.isResetConfiguration()) {
                    BraveVpnNativeWorker.getInstance().getSubscriberCredential(
                            BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT,
                            mBraveVpnPrefModel.getProductId(), BraveVpnUtils.IAP_ANDROID_PARAM_TEXT,
                            mBraveVpnPrefModel.getPurchaseToken(), getPackageName());
                } else {
                    mIsVerification = false;
                    showRestoreMenu(true);
                    Toast.makeText(BraveVpnParentActivity.this, R.string.already_subscribed,
                                 Toast.LENGTH_SHORT)
                            .show();
                    BraveVpnUtils.dismissProgressDialog();
                }
            } else {
                BraveVpnApiResponseUtils.queryPurchaseFailed(BraveVpnParentActivity.this);
                if (mIsVerification) {
                    mIsVerification = false;
                    showRestoreMenu(false);
                    BraveVpnUtils.dismissProgressDialog();
                } else {
                    BraveVpnUtils.openBraveVpnPlansActivity(BraveVpnParentActivity.this);
                }
            }
        } else {
            BraveVpnUtils.dismissProgressDialog();
        }
    };

    @Override
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
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
    public void onGetWireguardProfileCredentials(
            String jsonWireguardProfileCredentials, boolean isSuccess) {
        if (isSuccess && mBraveVpnPrefModel != null) {
            BraveVpnWireguardProfileCredentials braveVpnWireguardProfileCredentials =
                    BraveVpnUtils.getWireguardProfileCredentials(jsonWireguardProfileCredentials);
            if (BraveVpnProfileUtils.getInstance().isBraveVPNConnected(this)) {
                BraveVpnProfileUtils.getInstance().stopVpn(this);
            }
            mBraveVpnPrefModel.setClientId(braveVpnWireguardProfileCredentials.getClientId());
            mBraveVpnPrefModel.setApiAuthToken(
                    braveVpnWireguardProfileCredentials.getApiAuthToken());
            BraveVpnPrefUtils.setPrefModel(mBraveVpnPrefModel);

            checkForVpn(braveVpnWireguardProfileCredentials, mBraveVpnPrefModel);
        } else {
            Toast.makeText(this, R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG).show();
            BraveVpnUtils.dismissProgressDialog();
        }
    }

    private void checkForVpn(
            BraveVpnWireguardProfileCredentials braveVpnWireguardProfileCredentials,
            BraveVpnPrefModel braveVpnPrefModel) {
        new Thread() {
            @Override
            public void run() {
                try {
                    if (!WireguardConfigUtils.isConfigExist(getApplicationContext())) {
                        WireguardConfigUtils.createConfig(getApplicationContext(),
                                braveVpnWireguardProfileCredentials.getMappedIpv4Address(),
                                braveVpnPrefModel.getHostname(),
                                braveVpnPrefModel.getClientPrivateKey(),
                                braveVpnWireguardProfileCredentials.getServerPublicKey());
                    }

                    Intent intent = GoBackend.VpnService.prepare(BraveVpnParentActivity.this);
                    if (intent != null) {
                        intentActivityResultLauncher.launch(intent);
                        return;
                    }
                    BraveVpnUtils.dismissProgressDialog();
                    BraveVpnProfileUtils.getInstance().startVpn(BraveVpnParentActivity.this);
                    finish();
                } catch (Exception e) {
                    BraveVpnUtils.dismissProgressDialog();
                    Log.e(TAG, e.getMessage());
                }
            }
        }.start();
    }

    public OneshotSupplier<Profile> getProfileSupplier() {
        return mProfileSupplier;
    }
}
