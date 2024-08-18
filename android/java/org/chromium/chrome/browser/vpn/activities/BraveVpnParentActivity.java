/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn.activities;

import android.content.Intent;
import android.os.Handler;
import android.util.Pair;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import com.wireguard.android.backend.GoBackend;
import com.wireguard.crypto.KeyPair;

import org.chromium.base.Log;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.PurchaseModel;
import org.chromium.chrome.browser.init.ActivityProfileProvider;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.profiles.ProfileProvider;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.models.BraveVpnPrefModel;
import org.chromium.chrome.browser.vpn.models.BraveVpnWireguardProfileCredentials;
import org.chromium.chrome.browser.vpn.utils.BraveVpnApiResponseUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.wireguard.WireguardConfigUtils;

public abstract class BraveVpnParentActivity
        extends AsyncInitializationActivity implements BraveVpnObserver {
    private static final String TAG = "BraveVPN";
    public boolean mIsVerification;
    protected BraveVpnPrefModel mBraveVpnPrefModel;
    protected boolean mIsServerLocationChanged;

    private static final int INVALIDATE_CREDENTIAL_TIMER_COUNT = 5000;

    abstract void showRestoreMenu(boolean shouldShowRestore);
    abstract void updateProfileView();

    // Pass @{code ActivityResultRegistry} reference explicitly to avoid crash
    // https://github.com/brave/brave-browser/issues/31882
    ActivityResultLauncher<Intent> mIntentActivityResultLauncher =
            registerForActivityResult(
                    new ActivityResultContracts.StartActivityForResult(),
                    getActivityResultRegistry(),
                    result -> {
                        BraveVpnUtils.dismissProgressDialog();
                        if (result.getResultCode() == RESULT_OK) {
                            BraveVpnProfileUtils.getInstance()
                                    .startVpn(BraveVpnParentActivity.this);
                            BraveVpnUtils.showVpnConfirmDialog(this);
                        } else if (result.getResultCode() == RESULT_CANCELED) {
                            if (BraveVpnProfileUtils.getInstance().isVPNRunning(this)) {
                                BraveVpnUtils.showVpnAlwaysOnErrorDialog(this);
                            } else {
                                updateProfileView();
                            }
                            BraveVpnUtils.showToast(
                                    getResources().getString(R.string.permission_was_cancelled));
                        }
                    });

    @Override
    public void onResumeWithNative() {
        super.onResumeWithNative();
        BraveVpnNativeWorker.getInstance().addObserver(this);
    }

    @Override
    public void onPauseWithNative() {
        BraveVpnNativeWorker.getInstance().removeObserver(this);
        super.onPauseWithNative();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
    }

    @Override
    protected void onDestroy() {
        BraveVpnUtils.dismissProgressDialog();
        super.onDestroy();
    }

    protected void verifySubscription() {
        Log.e("brave_vpn", "verifySubscription : 1");
        mBraveVpnPrefModel = new BraveVpnPrefModel();
        MutableLiveData<PurchaseModel> _activePurchases = new MutableLiveData();
        LiveData<PurchaseModel> activePurchases = _activePurchases;
        InAppPurchaseWrapper.getInstance()
                .queryPurchases(_activePurchases, InAppPurchaseWrapper.SubscriptionProduct.VPN);
        LiveDataUtil.observeOnce(
                activePurchases,
                activePurchaseModel -> {
                    if (activePurchaseModel != null) {
                        Log.e("brave_vpn", "verifySubscription : 2");
                        mBraveVpnPrefModel.setPurchaseToken(activePurchaseModel.getPurchaseToken());
                        mBraveVpnPrefModel.setProductId(activePurchaseModel.getProductId());
                        BraveVpnNativeWorker.getInstance()
                                .verifyPurchaseToken(
                                        mBraveVpnPrefModel.getPurchaseToken(),
                                        mBraveVpnPrefModel.getProductId(),
                                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT,
                                        getPackageName());
                    } else {
                        Log.e("brave_vpn", "verifySubscription : 3");
                        if (!mIsVerification) {
                            BraveVpnApiResponseUtils.queryPurchaseFailed(
                                    BraveVpnParentActivity.this);
                            mIsServerLocationChanged = false;
                        } else {
                            showRestoreMenu(false);
                        }
                        BraveVpnUtils.dismissProgressDialog();
                    }
                });
    }

    @Override
    public void onVerifyPurchaseToken(
            String jsonResponse, String purchaseToken, String productId, boolean isSuccess) {
        Log.e("brave_vpn", "onVerifyPurchaseToken : 1");
        if (isSuccess && mBraveVpnPrefModel != null) {
            Log.e("brave_vpn", "onVerifyPurchaseToken : 2");
            Long purchaseExpiry = BraveVpnUtils.getPurchaseExpiryDate(jsonResponse);
            int paymentState = BraveVpnUtils.getPaymentState(jsonResponse);
            if (purchaseExpiry > 0 && purchaseExpiry >= System.currentTimeMillis()) {
                BraveVpnPrefUtils.setPurchaseToken(purchaseToken);
                BraveVpnPrefUtils.setProductId(productId);
                BraveVpnPrefUtils.setPurchaseExpiry(purchaseExpiry);
                BraveVpnPrefUtils.setSubscriptionPurchase(true);
                BraveVpnPrefUtils.setPaymentState(paymentState);
                if (!mIsVerification || BraveVpnPrefUtils.isResetConfiguration()) {
                    Log.e("brave_vpn", "onVerifyPurchaseToken : 3");
                    BraveVpnNativeWorker.getInstance()
                            .getSubscriberCredential(
                                    BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT,
                                    mBraveVpnPrefModel.getProductId(),
                                    BraveVpnUtils.IAP_ANDROID_PARAM_TEXT,
                                    mBraveVpnPrefModel.getPurchaseToken(),
                                    getPackageName());
                } else {
                    Log.e("brave_vpn", "onVerifyPurchaseToken : 4");
                    mIsVerification = false;
                    showRestoreMenu(true);
                    BraveVpnUtils.showToast(getResources().getString(R.string.already_subscribed));
                    BraveVpnUtils.dismissProgressDialog();
                }
            } else {
                Log.e("brave_vpn", "onVerifyPurchaseToken : 5");
                BraveVpnApiResponseUtils.queryPurchaseFailed(BraveVpnParentActivity.this);
                mIsServerLocationChanged = false;
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

            int timerCount = 0;
            if (mIsServerLocationChanged) {
                timerCount = INVALIDATE_CREDENTIAL_TIMER_COUNT;
                mIsServerLocationChanged = false;
                try {
                    BraveVpnNativeWorker.getInstance()
                            .invalidateCredentials(
                                    BraveVpnPrefUtils.getHostname(),
                                    BraveVpnPrefUtils.getClientId(),
                                    BraveVpnPrefUtils.getSubscriberCredential(),
                                    BraveVpnPrefUtils.getApiAuthToken());
                } catch (Exception ex) {
                    Log.e(TAG, ex.getMessage());
                }
            }

            new Handler()
                    .postDelayed(
                            () -> {
                                checkForVpn(braveVpnWireguardProfileCredentials);
                            },
                            timerCount);
        } else {
            BraveVpnUtils.showToast(getResources().getString(R.string.vpn_profile_creation_failed));
            BraveVpnUtils.dismissProgressDialog();
        }
    }

    public void changeServerRegion() {
        mIsServerLocationChanged = true;
        // BraveVpnUtils.selectedServerRegion = braveVpnServerRegion;
        BraveVpnUtils.showProgressDialog(
                BraveVpnParentActivity.this, getResources().getString(R.string.vpn_connect_text));
        if (BraveVpnNativeWorker.getInstance().isPurchasedUser()) {
            mBraveVpnPrefModel = new BraveVpnPrefModel();
            BraveVpnNativeWorker.getInstance().getSubscriberCredentialV12();
        } else {
            verifySubscription();
        }
    }

    private void checkForVpn(
            BraveVpnWireguardProfileCredentials braveVpnWireguardProfileCredentials) {
        new Thread() {
            @Override
            public void run() {
                try {
                    if (BraveVpnProfileUtils.getInstance()
                            .isBraveVPNConnected(BraveVpnParentActivity.this)) {
                        BraveVpnProfileUtils.getInstance().stopVpn(BraveVpnParentActivity.this);
                    }
                    if (WireguardConfigUtils.isConfigExist(getApplicationContext())) {
                        WireguardConfigUtils.deleteConfig(getApplicationContext());
                    }
                    WireguardConfigUtils.createConfig(
                            getApplicationContext(),
                            braveVpnWireguardProfileCredentials.getMappedIpv4Address(),
                            mBraveVpnPrefModel.getHostname(),
                            mBraveVpnPrefModel.getClientPrivateKey(),
                            braveVpnWireguardProfileCredentials.getServerPublicKey());

                    mBraveVpnPrefModel.setClientId(
                            braveVpnWireguardProfileCredentials.getClientId());
                    mBraveVpnPrefModel.setApiAuthToken(
                            braveVpnWireguardProfileCredentials.getApiAuthToken());
                    BraveVpnPrefUtils.setPrefModel(mBraveVpnPrefModel);

                    BraveVpnUtils.dismissProgressDialog();
                    Intent intent = GoBackend.VpnService.prepare(BraveVpnParentActivity.this);
                    if (intent != null) {
                        mIntentActivityResultLauncher.launch(intent);
                        return;
                    }
                    BraveVpnProfileUtils.getInstance().startVpn(BraveVpnParentActivity.this);
                    finish();
                } catch (Exception e) {
                    BraveVpnUtils.dismissProgressDialog();
                    Log.e(TAG, e.getMessage());
                }
            }
        }.start();
    }

    @Override
    protected OneshotSupplier<ProfileProvider> createProfileProvider() {
        return new ActivityProfileProvider(getLifecycleDispatcher());
    }
}
