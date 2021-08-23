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
import android.util.Pair;

import androidx.annotation.Nullable;
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

    private ChromeSwitchPreference mVpnSwitch;
    private ChromeBasePreference subscriptionStatus;
    private ChromeBasePreference subscriptionExpires;
    private ChromeBasePreference serverHost;
    private ChromeBasePreference serverLocation;
    private ListPreference serverLocationPref;
    private boolean isSubscriptionPurchased = false;
    private String subscriberCredential;
    private String hostname;
    private String productId;

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
                if (VpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
                    VpnProfileUtils.getInstance(getActivity()).stopVpn();
                } else {
                    if (BraveVpnUtils.isSubscriptionPurchased()) {
                        verifyPurchase(true);
                    } else {
                        BraveVpnUtils.openBraveVpnPlansActivity(getActivity());
                    }
                }
                return false;
            }
        });
        // mVpnSwitch.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
        //     @Override
        //     public boolean onPreferenceChange(Preference preference, Object newValue) {

        //         return false;
        //     }
        // });

        subscriptionStatus = (ChromeBasePreference) findPreference(PREF_SUBSCRIPTION_STATUS);
        subscriptionExpires = (ChromeBasePreference) findPreference(PREF_SUBSCRIPTION_EXPIRES);

        serverHost = (ChromeBasePreference) findPreference(PREF_SERVER_HOST);
        serverLocation = (ChromeBasePreference) findPreference(PREF_SERVER_LOCATION);
        serverHost.setSummary(BraveVpnUtils.getHostname());
        serverLocation.setSummary(BraveVpnUtils.getServerRegion(PREF_SERVER_CHANGE_LOCATION));

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

        serverLocationPref = (ListPreference) findPreference(PREF_SERVER_CHANGE_LOCATION);
        List<String> regions = new ArrayList<>();
        List<String> regionNames = new ArrayList<>();
        Collections.sort(BraveVpnUtils.vpnServerRegions, new Comparator<VpnServerRegion>() {
            @Override
            public int compare(VpnServerRegion vpnServerRegion1, VpnServerRegion vpnServerRegion2) {
                return vpnServerRegion1.getNamePretty().compareToIgnoreCase(
                        vpnServerRegion2.getNamePretty());
            }
        });
        serverLocationPref.setValue(BraveVpnUtils.getServerRegion(PREF_SERVER_CHANGE_LOCATION));
        for (VpnServerRegion vpnServerRegion : BraveVpnUtils.vpnServerRegions) {
            regions.add(vpnServerRegion.getName());
            regionNames.add(vpnServerRegion.getNamePretty());
        }
        serverLocationPref.setEntries(regionNames.toArray(new CharSequence[0]));
        serverLocationPref.setEntryValues(regions.toArray(new CharSequence[0]));
        serverLocationPref.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                Log.e("BraveVPN", "onPreferenceChange : " + (String) newValue);
                verifyPurchase(false);
                return true;
            }
        });
    }

    @Override
    public void onActivityCreated(@Nullable Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        // ConnectivityManager connectivityManager =
        //             (ConnectivityManager)
        //             getActivity().getSystemService(Context.CONNECTIVITY_SERVICE);
        //     NetworkRequest networkRequest =
        //             new NetworkRequest.Builder()
        //                     .addTransportType(NetworkCapabilities.TRANSPORT_VPN)
        //                     .removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VPN)
        //                     .build();
        //     connectivityManager.registerNetworkCallback(networkRequest, mNetworkCallback);

        verifyPurchase(true);
    }

    private final ConnectivityManager
            .NetworkCallback mNetworkCallback = new ConnectivityManager.NetworkCallback() {
        @Override
        public void onAvailable(Network network) {
            BraveVpnUtils.showBraveVpnNotification(getActivity());
            getActivity().runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (mVpnSwitch != null)
                        mVpnSwitch.setChecked(
                                VpnProfileUtils.getInstance(getActivity()).isVPNConnected());
                    if (serverHost != null) serverHost.setSummary(BraveVpnUtils.getHostname());
                    if (serverLocation != null)
                        serverLocation.setSummary(
                                BraveVpnUtils.getServerRegion(PREF_SERVER_CHANGE_LOCATION));
                }
            });
        }

        @Override
        public void onLost(Network network) {
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
    };

    private void verifyPurchase(boolean isVerification) {
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        if (purchases != null) {
            for (Purchase purchase : purchases) {
                if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                    String purchaseToken = purchase.getPurchaseToken();
                    productId = purchase.getSkus().get(0).toString();
                    Log.e("BraveVPN", "Purchase Token : " + purchaseToken);
                    isSubscriptionPurchased = true;
                    if (isVerification) {
                        BraveVpnNativeWorker.getInstance().verifyPurchaseToken(purchaseToken,
                                productId, "subscription", getActivity().getPackageName());
                    } else {
                        BraveVpnNativeWorker.getInstance().getSubscriberCredential("subscription",
                                productId, "iap-android", purchaseToken,
                                getActivity().getPackageName());
                    }
                }
            }
            if (!isSubscriptionPurchased) {
                BraveVpnUtils.openBraveVpnPlansActivity(getActivity());
            }
        }
    }

    @Override
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        boolean isPurchaseVerified =
                BraveVpnUtils.isPurchaseValid(jsonResponse) && isSubscriptionPurchased;
        Log.e("BraveVPN", "isPurchaseVerified : " + isPurchaseVerified);
        if (isPurchaseVerified) {
            if (subscriptionStatus != null) {
                subscriptionStatus.setSummary(
                        productId.equals(InAppPurchaseWrapper.NIGHTLY_MONTHLY_SUBSCRIPTION)
                                ? "Monthly Subscription"
                                : "Yearly Subscription");
            }

            if (subscriptionExpires != null) {
                long expiresInMillis =
                        Long.parseLong(BraveVpnUtils.getPurchaseExpiryDate(jsonResponse));
                SimpleDateFormat formatter =
                        new SimpleDateFormat("dd/MM/yyyy", Locale.getDefault());
                subscriptionExpires.setSummary(formatter.format(new Date(expiresInMillis)));
            }
            VpnProfileUtils.getInstance(getActivity()).startStopVpn();
        } else {
            Toast.makeText(getActivity(), R.string.purchase_token_verification_failed,
                         Toast.LENGTH_LONG)
                    .show();
            BraveVpnUtils.openBraveVpnPlansActivity(getActivity());
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
            String serverRegion = BraveVpnUtils.getServerRegion(PREF_SERVER_CHANGE_LOCATION);
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
            BraveVpnUtils.setHostname(hostname);
            if (VpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
                VpnProfileUtils.getInstance(getActivity()).stopVpn();
            }
            try {
                VpnProfileUtils.getInstance(getActivity())
                        .createVpnProfile(getActivity(), hostname, profileCredentials.first,
                                profileCredentials.second);
            } catch (Exception securityException) {
                Log.e("BraveVPN", "securityException");
                VpnProfileUtils.getInstance(getActivity()).startVpn();
            }
            // if (mVpnSwitch != null)
            // mVpnSwitch.setChecked(VpnProfileUtils.getInstance(getActivity()).isVPNConnected());
            if (serverHost != null) serverHost.setSummary(BraveVpnUtils.getHostname());
            if (serverLocation != null)
                serverLocation.setSummary(
                        BraveVpnUtils.getServerRegion(PREF_SERVER_CHANGE_LOCATION));
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
        // ConnectivityManager connectivityManager =
        //             (ConnectivityManager)
        //             getActivity().getSystemService(Context.CONNECTIVITY_SERVICE);
        //     NetworkRequest networkRequest =
        //             new NetworkRequest.Builder()
        //                     .addTransportType(NetworkCapabilities.TRANSPORT_VPN)
        //                     .removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VPN)
        //                     .build();
        //     connectivityManager.registerNetworkCallback(networkRequest, mNetworkCallback);
    }

    @Override
    public void onStop() {
        BraveVpnNativeWorker.getInstance().removeObserver(this);
        Log.e("BraveVPN", "BraveVpnPref : onStop");
        super.onStop();
    }
}
