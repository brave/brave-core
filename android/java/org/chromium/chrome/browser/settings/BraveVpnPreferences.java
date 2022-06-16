/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Pair;

import androidx.appcompat.app.AlertDialog;
import androidx.preference.Preference;

import com.android.billingclient.api.Purchase;
import com.wireguard.android.backend.GoBackend;
import com.wireguard.crypto.KeyPair;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.InternetConnection;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.activities.BraveVpnProfileActivity;
import org.chromium.chrome.browser.vpn.models.BraveVpnPrefModel;
import org.chromium.chrome.browser.vpn.models.BraveVpnServerRegion;
import org.chromium.chrome.browser.vpn.models.BraveVpnWireguardProfileCredentials;
import org.chromium.chrome.browser.vpn.utils.BraveVpnApiResponseUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.utils.InAppPurchaseWrapper;
import org.chromium.chrome.browser.vpn.wireguard.WireguardConfigUtils;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.ui.widget.Toast;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.TimeZone;

public class BraveVpnPreferences extends BravePreferenceFragment implements BraveVpnObserver {
    private static final String TAG = "BraveVPN";
    public static final String PREF_VPN_SWITCH = "vpn_switch";
    public static final String PREF_SUBSCRIPTION_MANAGE = "subscription_manage";
    public static final String PREF_SUBSCRIPTION_STATUS = "subscription_status";
    public static final String PREF_SUBSCRIPTION_EXPIRES = "subscription_expires";
    public static final String PREF_SERVER_HOST = "server_host";
    public static final String PREF_SERVER_CHANGE_LOCATION = "server_change_location";
    public static final String PREF_SUPPORT_TECHNICAL = "support_technical";
    public static final String PREF_SUPPORT_VPN = "support_vpn";
    public static final String PREF_SERVER_RESET_CONFIGURATION = "server_reset_configuration";

    private static final int INVALIDATE_CREDENTIAL_TIMER_COUNT = 5000;

    private static final String VPN_SUPPORT_PAGE =
            "https://support.brave.com/hc/en-us/articles/360045045952";
    private static final String MANAGE_SUBSCRIPTION_PAGE =
            "https://play.google.com/store/account/subscriptions";

    private static final String DATE_FORMAT = "dd/MM/yyyy";

    private ChromeSwitchPreference mVpnSwitch;
    private ChromeBasePreference mSubscriptionStatus;
    private ChromeBasePreference mSubscriptionExpires;
    private ChromeBasePreference mServerHost;
    private BraveVpnPrefModel mBraveVpnPrefModel;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.brave_firewall_vpn);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_vpn_preferences);

        if (BraveVpnUtils.isBraveVpnFeatureEnable()) {
            InAppPurchaseWrapper.getInstance().startBillingServiceConnection(getActivity());
        }

        mVpnSwitch = (ChromeSwitchPreference) findPreference(PREF_VPN_SWITCH);
        mVpnSwitch.setChecked(
                BraveVpnProfileUtils.getInstance().isBraveVPNConnected(getActivity()));
        mVpnSwitch.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                if (mVpnSwitch != null) {
                    mVpnSwitch.setChecked(
                            BraveVpnProfileUtils.getInstance().isBraveVPNConnected(getActivity()));
                }
                if (BraveVpnProfileUtils.getInstance().isBraveVPNConnected(getActivity())) {
                    BraveVpnUtils.showProgressDialog(
                            getActivity(), getResources().getString(R.string.vpn_disconnect_text));
                    BraveVpnProfileUtils.getInstance().stopVpn(getActivity());
                } else {
                    BraveVpnUtils.showProgressDialog(
                            getActivity(), getResources().getString(R.string.vpn_connect_text));
                    if (BraveVpnPrefUtils.isSubscriptionPurchase()) {
                        verifyPurchase(true);
                    } else {
                        BraveVpnUtils.openBraveVpnPlansActivity(getActivity());
                        BraveVpnUtils.dismissProgressDialog();
                    }
                }
                return false;
            }
        });

        mSubscriptionStatus = (ChromeBasePreference) findPreference(PREF_SUBSCRIPTION_STATUS);
        mSubscriptionExpires = (ChromeBasePreference) findPreference(PREF_SUBSCRIPTION_EXPIRES);

        mServerHost = (ChromeBasePreference) findPreference(PREF_SERVER_HOST);

        findPreference(PREF_SUPPORT_TECHNICAL)
                .setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                    @Override
                    public boolean onPreferenceClick(Preference preference) {
                        BraveVpnUtils.openBraveVpnSupportActivity(getActivity());
                        return true;
                    }
                });

        findPreference(PREF_SUPPORT_VPN)
                .setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                    @Override
                    public boolean onPreferenceClick(Preference preference) {
                        Intent browserIntent =
                                new Intent(Intent.ACTION_VIEW, Uri.parse(VPN_SUPPORT_PAGE));
                        getActivity().startActivity(browserIntent);
                        return true;
                    }
                });

        findPreference(PREF_SUBSCRIPTION_MANAGE)
                .setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                    @Override
                    public boolean onPreferenceClick(Preference preference) {
                        Intent browserIntent =
                                new Intent(Intent.ACTION_VIEW, Uri.parse(MANAGE_SUBSCRIPTION_PAGE));
                        getActivity().startActivity(browserIntent);
                        return true;
                    }
                });

        findPreference(PREF_SERVER_RESET_CONFIGURATION)
                .setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                    @Override
                    public boolean onPreferenceClick(Preference preference) {
                        showConfirmDialog();
                        return true;
                    }
                });
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context != null) {
            ConnectivityManager connectivityManager =
                    (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkRequest networkRequest =
                    new NetworkRequest.Builder()
                            .addTransportType(NetworkCapabilities.TRANSPORT_VPN)
                            .removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VPN)
                            .build();
            connectivityManager.registerNetworkCallback(networkRequest, mNetworkCallback);
        }
        BraveVpnNativeWorker.getInstance().getAllServerRegions();
        if (!InternetConnection.isNetworkAvailable(getActivity())) {
            Toast.makeText(getActivity(), R.string.no_internet, Toast.LENGTH_SHORT).show();
            getActivity().finish();
        }
    }

    @Override
    public void onGetAllServerRegions(String jsonResponse, boolean isSuccess) {
        if (isSuccess) {
            BraveVpnPrefUtils.setServerRegions(jsonResponse);
            new Handler().post(() -> updateSummaries());
        } else {
            Toast.makeText(getActivity(), R.string.fail_to_get_server_locations, Toast.LENGTH_LONG)
                    .show();
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        if (BraveVpnUtils.mIsServerLocationChanged) {
            BraveVpnUtils.showProgressDialog(
                    getActivity(), getResources().getString(R.string.vpn_connect_text));
            verifyPurchase(false);
        } else {
            BraveVpnUtils.dismissProgressDialog();
        }
    }

    private void updateSummary(String preferenceString, String summary) {
        Preference p = findPreference(preferenceString);
        p.setSummary(summary);
    }

    private void updateSummaries() {
        List<BraveVpnServerRegion> vpnServerRegions =
                BraveVpnUtils.getServerLocations(BraveVpnPrefUtils.getServerRegions());
        String serverLocation = "";
        for (BraveVpnServerRegion vpnServerRegion : vpnServerRegions) {
            if (BraveVpnPrefUtils.getServerRegion().equals(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC)) {
                serverLocation = getActivity().getResources().getString(R.string.automatic);
            }
            if (vpnServerRegion.getName().equals(BraveVpnPrefUtils.getServerRegion())) {
                serverLocation = vpnServerRegion.getNamePretty();
                break;
            }
        }
        updateSummary(PREF_SERVER_CHANGE_LOCATION, serverLocation);
        updateSummary(PREF_SERVER_HOST, BraveVpnPrefUtils.getHostnameDisplay());
        if (!BraveVpnPrefUtils.getProductId().isEmpty()) {
            String subscriptionStatus = String.format(
                    InAppPurchaseWrapper.getInstance().isMonthlySubscription(
                            BraveVpnPrefUtils.getProductId())
                            ? getActivity().getResources().getString(R.string.monthly_subscription)
                            : getActivity().getResources().getString(R.string.yearly_subscription),
                    (BraveVpnPrefUtils.isTrialSubscription()
                                    ? getActivity().getResources().getString(R.string.trial)
                                    : ""));
            updateSummary(PREF_SUBSCRIPTION_STATUS, subscriptionStatus);
        }

        if (BraveVpnPrefUtils.getPurchaseExpiry() > 0) {
            long expiresInMillis = BraveVpnPrefUtils.getPurchaseExpiry();
            SimpleDateFormat formatter = new SimpleDateFormat(DATE_FORMAT, Locale.getDefault());
            updateSummary(PREF_SUBSCRIPTION_EXPIRES, formatter.format(new Date(expiresInMillis)));
        }
        if (mVpnSwitch != null) {
            mVpnSwitch.setChecked(
                    BraveVpnProfileUtils.getInstance().isBraveVPNConnected(getActivity()));
        }
        new Thread() {
            @Override
            public void run() {
                if (getActivity() != null) {
                    getActivity().runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            findPreference(PREF_SERVER_CHANGE_LOCATION)
                                    .setEnabled(WireguardConfigUtils.isConfigExist(getActivity()));
                        }
                    });
                }
            }
        }.start();

        BraveVpnUtils.dismissProgressDialog();
    }

    private final ConnectivityManager.NetworkCallback mNetworkCallback =
            new ConnectivityManager.NetworkCallback() {
                @Override
                public void onAvailable(Network network) {
                    BraveVpnUtils.dismissProgressDialog();
                    if (getActivity() != null) {
                        getActivity().runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                new Handler().post(() -> updateSummaries());
                            }
                        });
                    }
                }

                @Override
                public void onLost(Network network) {
                    BraveVpnUtils.dismissProgressDialog();
                    if (getActivity() != null) {
                        getActivity().runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                new Handler().post(() -> updateSummaries());
                            }
                        });
                    }
                }
            };

    private void verifyPurchase(boolean isVerification) {
        mBraveVpnPrefModel = new BraveVpnPrefModel();
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        if (purchases != null && purchases.size() == 1) {
            Purchase purchase = purchases.get(0);
            mBraveVpnPrefModel.setPurchaseToken(purchase.getPurchaseToken());
            mBraveVpnPrefModel.setProductId(purchase.getSkus().get(0).toString());
            if (BraveVpnPrefUtils.isResetConfiguration()) {
                BraveVpnUtils.dismissProgressDialog();
                BraveVpnUtils.openBraveVpnProfileActivity(getActivity());
                return;
            }
            if (!isVerification) {
                BraveVpnNativeWorker.getInstance().getSubscriberCredential(
                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, mBraveVpnPrefModel.getProductId(),
                        BraveVpnUtils.IAP_ANDROID_PARAM_TEXT, mBraveVpnPrefModel.getPurchaseToken(),
                        getActivity().getPackageName());
            } else {
                BraveVpnNativeWorker.getInstance().verifyPurchaseToken(
                        mBraveVpnPrefModel.getPurchaseToken(), mBraveVpnPrefModel.getProductId(),
                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, getActivity().getPackageName());
            }
        } else {
            BraveVpnApiResponseUtils.queryPurchaseFailed(getActivity());
            BraveVpnUtils.mIsServerLocationChanged = false;
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
                if (mSubscriptionStatus != null) {
                    String subscriptionStatus = String.format(
                            InAppPurchaseWrapper.getInstance().isMonthlySubscription(
                                    BraveVpnPrefUtils.getProductId())
                                    ? getActivity().getResources().getString(
                                            R.string.monthly_subscription)
                                    : getActivity().getResources().getString(
                                            R.string.yearly_subscription),
                            (BraveVpnPrefUtils.isTrialSubscription()
                                            ? getActivity().getResources().getString(R.string.trial)
                                            : ""));
                    mSubscriptionStatus.setSummary(subscriptionStatus);
                }

                if (mSubscriptionExpires != null) {
                    SimpleDateFormat formatter =
                            new SimpleDateFormat(DATE_FORMAT, Locale.getDefault());
                    mSubscriptionExpires.setSummary(formatter.format(new Date(purchaseExpiry)));
                }
                checkVpnAfterVerification();
            } else {
                BraveVpnApiResponseUtils.queryPurchaseFailed(getActivity());
            }
        }
    };

    private void checkVpnAfterVerification() {
        new Thread() {
            @Override
            public void run() {
                Intent intent = GoBackend.VpnService.prepare(getActivity());
                if (intent != null || !WireguardConfigUtils.isConfigExist(getActivity())) {
                    BraveVpnUtils.dismissProgressDialog();
                    BraveVpnUtils.openBraveVpnProfileActivity(getActivity());
                    return;
                }
                BraveVpnProfileUtils.getInstance().startVpn(getActivity());
            }
        }.start();
    }

    @Override
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        mBraveVpnPrefModel.setSubscriberCredential(subscriberCredential);
        BraveVpnApiResponseUtils.handleOnGetSubscriberCredential(getActivity(), isSuccess);
    };

    @Override
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        BraveVpnApiResponseUtils.handleOnGetTimezonesForRegions(
                getActivity(), mBraveVpnPrefModel, jsonTimezones, isSuccess);
    }

    @Override
    public void onGetHostnamesForRegion(String jsonHostNames, boolean isSuccess) {
        KeyPair keyPair = new KeyPair();
        mBraveVpnPrefModel.setClientPrivateKey(keyPair.getPrivateKey().toBase64());
        mBraveVpnPrefModel.setClientPublicKey(keyPair.getPublicKey().toBase64());
        Pair<String, String> host = BraveVpnApiResponseUtils.handleOnGetHostnamesForRegion(
                getActivity(), mBraveVpnPrefModel, jsonHostNames, isSuccess);
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
            if (BraveVpnUtils.mIsServerLocationChanged) {
                timerCount = INVALIDATE_CREDENTIAL_TIMER_COUNT;
                BraveVpnUtils.mIsServerLocationChanged = false;
                try {
                    BraveVpnNativeWorker.getInstance().invalidateCredentials(
                            BraveVpnPrefUtils.getHostname(), BraveVpnPrefUtils.getClientId(),
                            BraveVpnPrefUtils.getSubscriberCredential(),
                            BraveVpnPrefUtils.getApiAuthToken());
                } catch (Exception ex) {
                    Log.e(TAG, ex.getMessage());
                }
            }

            new Handler().postDelayed(() -> {
                stopStartConnection(braveVpnWireguardProfileCredentials);
            }, timerCount);
        } else {
            Toast.makeText(getActivity(), R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            BraveVpnUtils.dismissProgressDialog();
            new Handler().post(() -> updateSummaries());
        }
    }

    private void stopStartConnection(
            BraveVpnWireguardProfileCredentials braveVpnWireguardProfileCredentials) {
        new Thread() {
            @Override
            public void run() {
                try {
                    if (BraveVpnProfileUtils.getInstance().isBraveVPNConnected(getActivity())) {
                        BraveVpnProfileUtils.getInstance().stopVpn(getActivity());
                    }
                    WireguardConfigUtils.deleteConfig(getActivity());
                    if (!WireguardConfigUtils.isConfigExist(getActivity())) {
                        WireguardConfigUtils.createConfig(getActivity(),
                                braveVpnWireguardProfileCredentials.getMappedIpv4Address(),
                                mBraveVpnPrefModel.getHostname(),
                                mBraveVpnPrefModel.getClientPrivateKey(),
                                braveVpnWireguardProfileCredentials.getServerPublicKey());
                    }
                    BraveVpnProfileUtils.getInstance().startVpn(getActivity());
                } catch (Exception e) {
                    Log.e(TAG, e.getMessage());
                }
                mBraveVpnPrefModel.setClientId(braveVpnWireguardProfileCredentials.getClientId());
                mBraveVpnPrefModel.setApiAuthToken(
                        braveVpnWireguardProfileCredentials.getApiAuthToken());
                BraveVpnPrefUtils.setPrefModel(mBraveVpnPrefModel);
                new Handler(Looper.getMainLooper()).post(() -> updateSummaries());
            }
        }.start();
    }

    @Override
    public void onStart() {
        super.onStart();
        BraveVpnNativeWorker.getInstance().addObserver(this);
    }

    @Override
    public void onStop() {
        BraveVpnNativeWorker.getInstance().removeObserver(this);
        super.onStop();
    }

    private void showConfirmDialog() {
        AlertDialog.Builder confirmDialog = new AlertDialog.Builder(getActivity());
        confirmDialog.setTitle(
                getActivity().getResources().getString(R.string.reset_vpn_config_dialog_title));
        confirmDialog.setMessage(
                getActivity().getResources().getString(R.string.reset_vpn_config_dialog_message));
        confirmDialog.setPositiveButton(
                getActivity().getResources().getString(android.R.string.yes), (dialog, which) -> {
                    BraveVpnNativeWorker.getInstance().invalidateCredentials(
                            BraveVpnPrefUtils.getHostname(), BraveVpnPrefUtils.getClientId(),
                            BraveVpnPrefUtils.getSubscriberCredential(),
                            BraveVpnPrefUtils.getApiAuthToken());
                    BraveVpnUtils.showProgressDialog(
                            getActivity(), getResources().getString(R.string.resetting_config));
                    new Handler().postDelayed(() -> {
                        BraveVpnUtils.resetProfileConfiguration(getActivity());
                        new Handler().post(() -> updateSummaries());
                    }, INVALIDATE_CREDENTIAL_TIMER_COUNT);
                    dialog.dismiss();
                });
        confirmDialog.setNegativeButton(getActivity().getResources().getString(android.R.string.no),
                (dialog, which) -> { dialog.dismiss(); });
        confirmDialog.show();
    }
}
