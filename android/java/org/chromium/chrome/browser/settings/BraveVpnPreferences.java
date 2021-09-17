/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.Uri;
import android.net.VpnManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Pair;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.Preference.OnPreferenceChangeListener;

import com.android.billingclient.api.Purchase;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRelaunchUtils;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.BraveRewardsPanelPopup;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.vpn.BraveVpnConfirmDialogFragment;
import org.chromium.chrome.browser.vpn.BraveVpnNativeWorker;
import org.chromium.chrome.browser.vpn.BraveVpnObserver;
import org.chromium.chrome.browser.vpn.BraveVpnPrefUtils;
import org.chromium.chrome.browser.vpn.BraveVpnUtils;
import org.chromium.chrome.browser.vpn.InAppPurchaseWrapper;
import org.chromium.chrome.browser.vpn.VpnProfileUtils;
import org.chromium.chrome.browser.vpn.VpnServerRegion;
import org.chromium.components.browser_ui.settings.ChromeBasePreference;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.ui.widget.Toast;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.TimeZone;

public class BraveVpnPreferences
        extends BravePreferenceFragment implements BraveRewardsObserver, BraveVpnObserver {
    public static final String PREF_VPN_SWITCH = "vpn_switch";
    public static final String PREF_SUBSCRIPTION_MANAGE = "subscription_manage";
    public static final String PREF_SERVER_CHANGE_LOCATION = "server_change_location";
    public static final String PREF_SUBSCRIPTION_STATUS = "subscription_status";
    public static final String PREF_SUBSCRIPTION_EXPIRES = "subscription_expires";
    public static final String PREF_SERVER_HOST = "server_host";
    public static final String PREF_SERVER_LOCATION = "server_location";
    public static final String PREF_SUPPORT_TECHNICAL = "support_technical";
    public static final String PREF_SUPPORT_VPN = "support_vpn";
    public static final String PREF_SERVER_RESET_CONFIGURATION = "server_reset_configuration";

    private ChromeSwitchPreference mVpnSwitch;
    private ChromeBasePreference subscriptionStatus;
    private ChromeBasePreference subscriptionExpires;
    private ChromeBasePreference serverHost;
    private ChromeBasePreference serverLocation;
    // private ListPreference serverLocationPref;
    private String subscriberCredential;
    private String hostname;
    private String purchaseToken = "";
    private String productId = "";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.brave_firewall_vpn);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_vpn_preferences);

        InAppPurchaseWrapper.getInstance().startBillingServiceConnection(getActivity());

        mVpnSwitch = (ChromeSwitchPreference) findPreference(PREF_VPN_SWITCH);
        mVpnSwitch.setChecked(VpnProfileUtils.getInstance(getActivity()).isVPNConnected());
        mVpnSwitch.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
            @Override
            public boolean onPreferenceClick(Preference preference) {
                if (mVpnSwitch != null) {
                    mVpnSwitch.setChecked(
                            VpnProfileUtils.getInstance(getActivity()).isVPNConnected());
                }
                if (VpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
                    VpnProfileUtils.getInstance(getActivity()).stopVpn();
                } else {
                    if (BraveVpnPrefUtils.isBraveVpnBooleanPref(
                                BraveVpnPrefUtils.PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, false)) {
                        verifyPurchase(true);
                    } else {
                        BraveVpnUtils.openBraveVpnPlansActivity(getActivity());
                    }
                }
                return false;
            }
        });

        subscriptionStatus = (ChromeBasePreference) findPreference(PREF_SUBSCRIPTION_STATUS);
        subscriptionExpires = (ChromeBasePreference) findPreference(PREF_SUBSCRIPTION_EXPIRES);

        serverHost = (ChromeBasePreference) findPreference(PREF_SERVER_HOST);
        serverLocation = (ChromeBasePreference) findPreference(PREF_SERVER_LOCATION);

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
                        BraveVpnUtils.openBraveVpnSupportActivity(getActivity());
                        return true;
                    }
                });

        findPreference(PREF_SUBSCRIPTION_MANAGE)
                .setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                    @Override
                    public boolean onPreferenceClick(Preference preference) {
                        Intent browserIntent = new Intent(Intent.ACTION_VIEW,
                                Uri.parse("https://play.google.com/store/account/subscriptions"));
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
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        // verifyPurchase(true);
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
    }

    @Override
    public void onResume() {
        super.onResume();
        if (BraveVpnUtils.isServerLocationChanged) {
            BraveVpnUtils.isServerLocationChanged = false;
            verifyPurchase(false);
        }
        new Handler().post(() -> updateSummaries());
    }

    private void updateSummary(String preferenceString, String summary) {
        Preference p = findPreference(preferenceString);
        p.setSummary(summary);
    }

    private void updateSummaries() {
        String serverLocation = "";
        for (VpnServerRegion vpnServerRegion : BraveVpnUtils.vpnServerRegions) {
            if (BraveVpnPrefUtils.getServerRegion(PREF_SERVER_CHANGE_LOCATION)
                            .equals("automatic")) {
                serverLocation = "Automatic";
            }
            if (vpnServerRegion.getName().equals(
                        BraveVpnPrefUtils.getServerRegion(PREF_SERVER_CHANGE_LOCATION))) {
                serverLocation = vpnServerRegion.getNamePretty();
                break;
            }
        }
        updateSummary(PREF_SERVER_LOCATION, serverLocation);
        updateSummary(PREF_SERVER_CHANGE_LOCATION, serverLocation);
        updateSummary(PREF_SERVER_HOST,
                BraveVpnPrefUtils.getBraveVpnStringPref(BraveVpnPrefUtils.PREF_BRAVE_VPN_HOSTNAME));
        if (!BraveVpnPrefUtils.getBraveVpnStringPref(BraveVpnPrefUtils.PREF_BRAVE_VPN_PRODUCT_ID)
                        .isEmpty()) {
            updateSummary(PREF_SUBSCRIPTION_STATUS,
                    BraveVpnPrefUtils
                                    .getBraveVpnStringPref(
                                            BraveVpnPrefUtils.PREF_BRAVE_VPN_PRODUCT_ID)
                                    .equals(InAppPurchaseWrapper.NIGHTLY_MONTHLY_SUBSCRIPTION)
                            ? "Monthly Subscription"
                            : "Yearly Subscription");
        }

        if (!BraveVpnPrefUtils
                        .getBraveVpnStringPref(BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_EXPIRY)
                        .isEmpty()) {
            long expiresInMillis = Long.parseLong(BraveVpnPrefUtils.getBraveVpnStringPref(
                    BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_EXPIRY));
            SimpleDateFormat formatter = new SimpleDateFormat("dd/MM/yyyy", Locale.getDefault());
            updateSummary(PREF_SUBSCRIPTION_EXPIRES, formatter.format(new Date(expiresInMillis)));
        }
        if (mVpnSwitch != null) {
            mVpnSwitch.setChecked(VpnProfileUtils.getInstance(getActivity()).isVPNConnected());
        }
    }

    private final ConnectivityManager
            .NetworkCallback mNetworkCallback = new ConnectivityManager.NetworkCallback() {
        @Override
        public void onAvailable(Network network) {
            Log.e("BraveVPN", "BraveVpnPreferences : onAvailable");
            if (getActivity() != null) {
                BraveVpnUtils.showBraveVpnNotification(getActivity());
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (mVpnSwitch != null)
                            mVpnSwitch.setChecked(
                                    VpnProfileUtils.getInstance(getActivity()).isVPNConnected());
                        updateSummaries();
                    }
                });
            }
        }

        @Override
        public void onLost(Network network) {
            Log.e("BraveVPN", "BraveVpnPreferences : onLost");
            if (getActivity() != null) {
                BraveVpnUtils.cancelBraveVpnNotification(getActivity());
                getActivity().runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (mVpnSwitch != null)
                            mVpnSwitch.setChecked(
                                    VpnProfileUtils.getInstance(getActivity()).isVPNConnected());
                    }
                });
            }
        }
    };

    private void verifyPurchase(boolean isVerification) {
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        if (purchases.size() == 1) {
            Purchase purchase = purchases.get(0);
            purchaseToken = purchase.getPurchaseToken();
            productId = purchase.getSkus().get(0).toString();
            Log.e("BraveVPN", "Purchase Token : " + purchaseToken);
            if (isVerification) {
                BraveVpnNativeWorker.getInstance().verifyPurchaseToken(
                        purchaseToken, productId, "subscription", getActivity().getPackageName());
            } else {
                BraveVpnNativeWorker.getInstance().getSubscriberCredential("subscription",
                        productId, "iap-android", purchaseToken, getActivity().getPackageName());
            }
        } else {
            BraveVpnPrefUtils.setBraveVpnStringPref(
                    BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_TOKEN, "");
            BraveVpnPrefUtils.setBraveVpnStringPref(
                    BraveVpnPrefUtils.PREF_BRAVE_VPN_PRODUCT_ID, "");
            BraveVpnPrefUtils.setBraveVpnStringPref(
                    BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_EXPIRY, "");
            BraveVpnPrefUtils.setBraveVpnBooleanPref(
                    BraveVpnPrefUtils.PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, false);
            if (VpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
                VpnProfileUtils.getInstance(getActivity()).stopVpn();
            }
            VpnProfileUtils.getInstance(getActivity()).deleteVpnProfile();
            BraveVpnUtils.openBraveVpnPlansActivity(getActivity());
        }
    }

    @Override
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        if (isSuccess) {
            String purchaseExpiry = BraveVpnUtils.getPurchaseExpiryDate(jsonResponse);
            if (!purchaseExpiry.isEmpty()
                    && Long.parseLong(purchaseExpiry) >= System.currentTimeMillis()) {
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_TOKEN, purchaseToken);
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PRODUCT_ID, productId);
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_EXPIRY, purchaseExpiry);
                BraveVpnPrefUtils.setBraveVpnBooleanPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, true);

                if (subscriptionStatus != null) {
                    subscriptionStatus.setSummary(
                            productId.equals(InAppPurchaseWrapper.NIGHTLY_MONTHLY_SUBSCRIPTION)
                                    ? "Monthly Subscription"
                                    : "Yearly Subscription");
                }

                if (subscriptionExpires != null) {
                    long expiresInMillis = Long.parseLong(purchaseExpiry);
                    SimpleDateFormat formatter =
                            new SimpleDateFormat("dd/MM/yyyy", Locale.getDefault());
                    subscriptionExpires.setSummary(formatter.format(new Date(expiresInMillis)));
                }

                VpnProfileUtils.getInstance(getActivity()).startStopVpn();
            } else {
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_TOKEN, "");
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PRODUCT_ID, "");
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_EXPIRY, "");
                BraveVpnPrefUtils.setBraveVpnBooleanPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_SUBSCRIPTION_PURCHASE, false);
                if (VpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
                    VpnProfileUtils.getInstance(getActivity()).stopVpn();
                }
                VpnProfileUtils.getInstance(getActivity()).deleteVpnProfile();
                Toast.makeText(getActivity(), R.string.purchase_token_verification_failed,
                             Toast.LENGTH_LONG)
                        .show();
                BraveVpnUtils.openBraveVpnPlansActivity(getActivity());
            }
            purchaseToken = "";
            productId = "";
        }
    };

    @Override
    public void onGetSubscriberCredential(String subscriberCredential, boolean isSuccess) {
        Log.e("BraveVPN", "isSuccess : " + isSuccess);
        if (isSuccess) {
            InAppPurchaseWrapper.getInstance().processPurchases(
                    InAppPurchaseWrapper.getInstance().queryPurchases());
            this.subscriberCredential = subscriberCredential;
            BraveVpnNativeWorker.getInstance().getTimezonesForRegions();
        } else {
            Toast.makeText(getActivity(), R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "onGetSubscriberCredential : failed");
        }
    };

    @Override
    public void onGetTimezonesForRegions(String jsonTimezones, boolean isSuccess) {
        Log.e("BraveVPN", jsonTimezones);
        if (isSuccess) {
            String region = BraveVpnUtils.getRegionForTimeZone(
                    jsonTimezones, TimeZone.getDefault().getID());
            String serverRegion = BraveVpnPrefUtils.getServerRegion(PREF_SERVER_CHANGE_LOCATION);
            Log.e("BraveVPN",
                    "Region : " + (serverRegion.equals("automatic") ? region : serverRegion));
            BraveVpnNativeWorker.getInstance().getHostnamesForRegion(
                    serverRegion.equals("automatic") ? region : serverRegion);
        } else {
            Toast.makeText(getActivity(), R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "onGetTimezonesForRegions : failed");
        }
    }

    @Override
    public void onGetHostnamesForRegion(String jsonHostNames, boolean isSuccess) {
        Log.e("BraveVPN", jsonHostNames);
        if (isSuccess) {
            hostname = BraveVpnUtils.getHostnameForRegion(jsonHostNames);
            Log.e("BraveVPN", "Hostname : " + hostname);
            BraveVpnNativeWorker.getInstance().getProfileCredentials(
                    subscriberCredential, hostname);
        } else {
            Toast.makeText(getActivity(), R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();

            Log.e("BraveVPN", "onGetHostnamesForRegion : failed");
        }
    }

    @Override
    public void onGetProfileCredentials(String jsonProfileCredentials, boolean isSuccess) {
        Log.e("BraveVPN", jsonProfileCredentials);
        if (isSuccess) {
            Pair<String, String> profileCredentials =
                    BraveVpnUtils.getProfileCredentials(jsonProfileCredentials);
            BraveVpnPrefUtils.setBraveVpnStringPref(
                    BraveVpnPrefUtils.PREF_BRAVE_VPN_HOSTNAME, hostname);
            if (VpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
                VpnProfileUtils.getInstance(getActivity()).stopVpn();
            }
            try {
                VpnProfileUtils.getInstance(getActivity())
                        .createVpnProfile(getActivity(), hostname, profileCredentials.first,
                                profileCredentials.second);
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PURCHASE_TOKEN, purchaseToken);
                BraveVpnPrefUtils.setBraveVpnStringPref(
                        BraveVpnPrefUtils.PREF_BRAVE_VPN_PRODUCT_ID, productId);
                updateSummaries();
            } catch (Exception securityException) {
                Log.e("BraveVPN", "securityException");
                VpnProfileUtils.getInstance(getActivity()).startVpn();
            }
            purchaseToken = "";
            productId = "";
            updateSummaries();
        } else {
            Toast.makeText(getActivity(), R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();
            Log.e("BraveVPN", "jsonProfileCredentials : failed");
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
        Log.e("BraveVPN", "BraveVpnPref : onStop");
        super.onStop();
    }

    private void showConfirmDialog() {
        AlertDialog.Builder confirmDialog = new AlertDialog.Builder(getActivity());

        confirmDialog.setTitle("Reset VPN configuration");

        confirmDialog.setMessage("Are you sure you want to reset vpn configuration?");

        confirmDialog.setPositiveButton("Yes", (dialog, which) -> {
            if (VpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
                VpnProfileUtils.getInstance(getActivity()).stopVpn();
            }
            VpnProfileUtils.getInstance(getActivity()).deleteVpnProfile();
            dialog.dismiss();
        });
        confirmDialog.setNegativeButton("No", (dialog, which) -> { dialog.dismiss(); });

        confirmDialog.show();
    }
}
