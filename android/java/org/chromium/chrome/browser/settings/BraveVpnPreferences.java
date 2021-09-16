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
import android.util.Pair;

import androidx.appcompat.app.AlertDialog;
import androidx.preference.Preference;

import com.android.billingclient.api.Purchase;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnProfileUtils;
import org.chromium.chrome.browser.vpn.BraveVpnServerRegion;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.InAppPurchaseWrapper;
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
    public static final String PREF_VPN_SWITCH = "vpn_switch";
    public static final String PREF_SUBSCRIPTION_MANAGE = "subscription_manage";
    public static final String PREF_SUBSCRIPTION_STATUS = "subscription_status";
    public static final String PREF_SUBSCRIPTION_EXPIRES = "subscription_expires";
    public static final String PREF_SERVER_HOST = "server_host";
    public static final String PREF_SERVER_LOCATION = "server_location";
    public static final String PREF_SUPPORT_TECHNICAL = "support_technical";
    public static final String PREF_SUPPORT_VPN = "support_vpn";
    public static final String PREF_SERVER_RESET_CONFIGURATION = "server_reset_configuration";

    private static final String VPN_SUPPORT_PAGE =
            "https://support.brave.com/hc/en-us/articles/360045045952";
    private static final String MANAGE_SUBSCRIPTION_PAGE =
            "https://play.google.com/store/account/subscriptions";

    private static final String DATE_FORMAT = "dd/MM/yyyy";

    private ChromeSwitchPreference mVpnSwitch;
    private ChromeBasePreference mSubscriptionStatus;
    private ChromeBasePreference mSubscriptionExpires;
    private ChromeBasePreference mServerHost;
    private ChromeBasePreference mServerLocation;
    private String mSubscriberCredential;
    private String mHostname;
    private String mPurchaseToken = "";
    private String mProductId = "";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.brave_firewall_vpn);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_vpn_preferences);

        InAppPurchaseWrapper.getInstance().startBillingServiceConnection(getActivity());

        mVpnSwitch = (ChromeSwitchPreference) findPreference(PREF_VPN_SWITCH);
        mVpnSwitch.setChecked(BraveVpnProfileUtils.getInstance(getActivity()).isVPNConnected());
        mVpnSwitch.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                BraveVpnUtils.showProgressDialog(getActivity());
                if (mVpnSwitch != null) {
                    mVpnSwitch.setChecked(
                            BraveVpnProfileUtils.getInstance(getActivity()).isVPNConnected());
                }
                if (BraveVpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
                    BraveVpnProfileUtils.getInstance(getActivity()).stopVpn();
                } else {
                    if (BraveVpnPrefUtils.isSubscriptionPurchase()) {
                        verifyPurchase(true);
                    } else {
                        BraveVpnUtils.openBraveVpnPlansActivity(getActivity());
                    }
                }
                return false;
            }
        });

        mSubscriptionStatus = (ChromeBasePreference) findPreference(PREF_SUBSCRIPTION_STATUS);
        mSubscriptionExpires = (ChromeBasePreference) findPreference(PREF_SUBSCRIPTION_EXPIRES);

        mServerHost = (ChromeBasePreference) findPreference(PREF_SERVER_HOST);
        mServerLocation = (ChromeBasePreference) findPreference(PREF_SERVER_LOCATION);

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
            BraveVpnUtils.mIsServerLocationChanged = false;
            BraveVpnUtils.showProgressDialog(getActivity());
            verifyPurchase(false);
        } else {
            BraveVpnUtils.dismissProgressDialog();
        }
        new Handler().post(() -> updateSummaries());
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
        updateSummary(PREF_SERVER_LOCATION, serverLocation);
        updateSummary(PREF_SERVER_HOST, BraveVpnPrefUtils.getHostname());
        if (!BraveVpnPrefUtils.getProductId().isEmpty()) {
            updateSummary(PREF_SUBSCRIPTION_STATUS,
                    BraveVpnPrefUtils.getProductId().equals(
                            InAppPurchaseWrapper.NIGHTLY_MONTHLY_SUBSCRIPTION)
                            ? getActivity().getResources().getString(R.string.monthly_subscription)
                            : getActivity().getResources().getString(R.string.yearly_subscription));
        }

        if (BraveVpnPrefUtils.getPurchaseExpiry() > 0) {
            long expiresInMillis = BraveVpnPrefUtils.getPurchaseExpiry();
            SimpleDateFormat formatter = new SimpleDateFormat(DATE_FORMAT, Locale.getDefault());
            updateSummary(PREF_SUBSCRIPTION_EXPIRES, formatter.format(new Date(expiresInMillis)));
        }
        if (mVpnSwitch != null) {
            mVpnSwitch.setChecked(BraveVpnProfileUtils.getInstance(getActivity()).isVPNConnected());
        }
    }

    private final ConnectivityManager.NetworkCallback mNetworkCallback =
            new ConnectivityManager.NetworkCallback() {
                @Override
                public void onAvailable(Network network) {
                    BraveVpnUtils.dismissProgressDialog();
                    if (getActivity() != null) {
                        BraveVpnUtils.showBraveVpnNotification(getActivity());
                        getActivity().runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if (mVpnSwitch != null)
                                    mVpnSwitch.setChecked(
                                            BraveVpnProfileUtils.getInstance(getActivity())
                                                    .isVPNConnected());
                                updateSummaries();
                            }
                        });
                    }
                }

                @Override
                public void onLost(Network network) {
                    BraveVpnUtils.dismissProgressDialog();
                    if (getActivity() != null) {
                        BraveVpnUtils.cancelBraveVpnNotification(getActivity());
                        getActivity().runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if (mVpnSwitch != null)
                                    mVpnSwitch.setChecked(
                                            BraveVpnProfileUtils.getInstance(getActivity())
                                                    .isVPNConnected());
                            }
                        });
                    }
                }
            };

    private void verifyPurchase(boolean isVerification) {
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        if (purchases.size() == 1) {
            Purchase purchase = purchases.get(0);
            mPurchaseToken = purchase.getPurchaseToken();
            mProductId = purchase.getSkus().get(0).toString();
            if (isVerification) {
                BraveVpnNativeWorker.getInstance().verifyPurchaseToken(mPurchaseToken, mProductId,
                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, getActivity().getPackageName());
            } else {
                BraveVpnNativeWorker.getInstance().getSubscriberCredential(
                        BraveVpnUtils.SUBSCRIPTION_PARAM_TEXT, mProductId,
                        BraveVpnUtils.IAP_ANDROID_PARAM_TEXT, mPurchaseToken,
                        getActivity().getPackageName());
            }
        } else {
            braveVpnVerificationFailed();
        }
    }

    @Override
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        if (isSuccess) {
            Long purchaseExpiry = BraveVpnUtils.getPurchaseExpiryDate(jsonResponse);
            if (purchaseExpiry > 0 && purchaseExpiry >= System.currentTimeMillis()) {
                BraveVpnPrefUtils.setPurchaseToken(mPurchaseToken);
                BraveVpnPrefUtils.setProductId(mProductId);
                BraveVpnPrefUtils.setPurchaseExpiry(purchaseExpiry);
                BraveVpnPrefUtils.setSubscriptionPurchase(true);

                if (mSubscriptionStatus != null) {
                    mSubscriptionStatus.setSummary(
                            mProductId.equals(InAppPurchaseWrapper.NIGHTLY_MONTHLY_SUBSCRIPTION)
                                    ? getActivity().getResources().getString(
                                            R.string.monthly_subscription)
                                    : getActivity().getResources().getString(
                                            R.string.yearly_subscription));
                }

                if (mSubscriptionExpires != null) {
                    SimpleDateFormat formatter =
                            new SimpleDateFormat(DATE_FORMAT, Locale.getDefault());
                    mSubscriptionExpires.setSummary(formatter.format(new Date(purchaseExpiry)));
                }

                BraveVpnProfileUtils.getInstance(getActivity()).startStopVpn();
            } else {
                braveVpnVerificationFailed();
            }
            mPurchaseToken = "";
            mProductId = "";
        }
    };

    private void braveVpnVerificationFailed() {
        BraveVpnPrefUtils.setPurchaseToken("");
        BraveVpnPrefUtils.setProductId("");
        BraveVpnPrefUtils.setPurchaseExpiry(0L);
        BraveVpnPrefUtils.setSubscriptionPurchase(false);
        if (BraveVpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
            BraveVpnProfileUtils.getInstance(getActivity()).stopVpn();
        }
        BraveVpnProfileUtils.getInstance(getActivity()).deleteVpnProfile();
        Toast.makeText(
                     getActivity(), R.string.purchase_token_verification_failed, Toast.LENGTH_LONG)
                .show();
        BraveVpnUtils.openBraveVpnPlansActivity(getActivity());
    }

    @Override
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        if (isSuccess) {
            InAppPurchaseWrapper.getInstance().processPurchases(
                    InAppPurchaseWrapper.getInstance().queryPurchases());
            this.mSubscriberCredential = subscriberCredential;
            BraveVpnNativeWorker.getInstance().getTimezonesForRegions();
        } else {
            Toast.makeText(getActivity(), R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "BraveVpnPreferences -> onGetSubscriberCredential : failed");
        }
    };

    @Override
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        if (isSuccess) {
            String region = BraveVpnUtils.getRegionForTimeZone(
                    jsonTimezones, TimeZone.getDefault().getID());
            String serverRegion = BraveVpnPrefUtils.getServerRegion();
            BraveVpnNativeWorker.getInstance().getHostnamesForRegion(
                    serverRegion.equals(BraveVpnPrefUtils.PREF_BRAVE_VPN_AUTOMATIC) ? region
                                                                                    : serverRegion);
        } else {
            Toast.makeText(getActivity(), R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "BraveVpnPreferences -> onGetTimezonesForRegions : failed");
        }
    }

    @Override
    public void onGetHostnamesForRegion(String jsonHostNames, boolean isSuccess) {
        if (isSuccess) {
            mHostname = BraveVpnUtils.getHostnameForRegion(jsonHostNames);
            BraveVpnNativeWorker.getInstance().getProfileCredentials(
                    mSubscriberCredential, mHostname);
        } else {
            Toast.makeText(getActivity(), R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "BraveVpnPreferences -> onGetHostnamesForRegion : failed");
        }
    }

    @Override
    public void onGetProfileCredentials(String jsonProfileCredentials, boolean isSuccess) {
        if (isSuccess) {
            Pair<String, String> profileCredentials =
                    BraveVpnUtils.getProfileCredentials(jsonProfileCredentials);
            BraveVpnPrefUtils.setHostname(mHostname);
            if (BraveVpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
                BraveVpnProfileUtils.getInstance(getActivity()).stopVpn();
            }
            try {
                BraveVpnProfileUtils.getInstance(getActivity())
                        .createVpnProfile(getActivity(), mHostname, profileCredentials.first,
                                profileCredentials.second);
                BraveVpnPrefUtils.setPurchaseToken(mPurchaseToken);
                BraveVpnPrefUtils.setProductId(mProductId);
                updateSummaries();
            } catch (Exception securityException) {
                BraveVpnProfileUtils.getInstance(getActivity()).startVpn();
            }
            mPurchaseToken = "";
            mProductId = "";
            updateSummaries();
        } else {
            Toast.makeText(getActivity(), R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "BraveVpnPreferences -> jsonProfileCredentials : failed");
        }
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
                    if (BraveVpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
                        BraveVpnProfileUtils.getInstance(getActivity()).stopVpn();
                    }
                    BraveVpnProfileUtils.getInstance(getActivity()).deleteVpnProfile();
                    dialog.dismiss();
                });
        confirmDialog.setNegativeButton(getActivity().getResources().getString(android.R.string.no),
                (dialog, which) -> { dialog.dismiss(); });
        confirmDialog.show();
    }
}
