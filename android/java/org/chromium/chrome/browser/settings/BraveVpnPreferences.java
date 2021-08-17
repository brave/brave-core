/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.net.VpnManager;
import android.os.Bundle;
import android.util.Pair;

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
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.TimeZone;

public class BraveVpnPreferences extends BravePreferenceFragment
        implements OnPreferenceChangeListener, BraveRewardsObserver, BraveVpnObserver {
    public static final String PREF_VPN_SWITCH = "vpn_switch";
    public static final String PREF_SUBSCRIPTION_MANAGE = "subscription_manage";
    public static final String PREF_SERVER_CHANGE_LOCATION = "server_change_location";

    private ChromeSwitchPreference mVpnSwitch;
    private ListPreference serverLocationPref;
    private boolean isSubscriptionPurchased = false;
    private String subscriberCredential;
    private String hostname;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.brave_firewall_vpn);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_vpn_preferences);

        InAppPurchaseWrapper.getInstance().startBillingServiceConnection(getActivity());

        mVpnSwitch = (ChromeSwitchPreference) findPreference(PREF_VPN_SWITCH);
        mVpnSwitch.setChecked(VpnProfileUtils.getInstance(getActivity()).isVPNConnected());
        mVpnSwitch.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                if (VpnProfileUtils.getInstance(getActivity()).isVPNConnected()) {
                    VpnProfileUtils.getInstance(getActivity()).stopVpn();
                } else {
                    if (BraveVpnUtils.isSubscriptionPurchased()) {
                        getPurchaseDetails(true);
                    } else {
                        BraveVpnUtils.openBraveVpnPlansActivity(getActivity());
                    }
                }
                return false;
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
                getPurchaseDetails(false);
                return true;
            }
        });
    }

    private void getPurchaseDetails(boolean isVerification) {
        List<Purchase> purchases = InAppPurchaseWrapper.getInstance().queryPurchases();
        for (Purchase purchase : purchases) {
            if (purchase.getPurchaseState() == Purchase.PurchaseState.PURCHASED) {
                String purchaseToken = purchase.getPurchaseToken();
                String productId = purchase.getSkus().get(0).toString();
                Log.e("BraveVPN", "Purchase Token : " + purchaseToken);
                isSubscriptionPurchased = true;
                if (isVerification) {
                    BraveVpnNativeWorker.getInstance().verifyPurchaseToken(purchaseToken, productId,
                            "subscription", getActivity().getPackageName());
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

    @Override
    public void onVerifyPurchaseToken(String jsonResponse, boolean isSuccess) {
        boolean isPurchaseVerified =
                BraveVpnUtils.isPurchaseValid(jsonResponse) && isSubscriptionPurchased;
        Log.e("BraveVPN", "isPurchaseVerified : " + isPurchaseVerified);
        if (isPurchaseVerified) {
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
        } else {
            Toast.makeText(getActivity(), R.string.vpn_profile_creation_failed, Toast.LENGTH_LONG)
                    .show();

            Log.e("BraveVPN", "jsonProfileCredentials : failed");
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        return true;
    }

    @Override
    public void onStart() {
        BraveVpnNativeWorker.getInstance().addObserver(this);
        super.onStart();
    }

    @Override
    public void onStop() {
        BraveVpnNativeWorker.getInstance().removeObserver(this);
        super.onStop();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        // super.onActivityResult(requestCode, resultCode, data); comment this unless you want to
        // pass your result to the activity.
        VpnManager vpnManager =
                (VpnManager) getActivity().getSystemService(Context.VPN_MANAGEMENT_SERVICE);
        if (vpnManager != null) {
            vpnManager.startProvisionedVpnProfile();
            // BraveVpnConfirmDialogFragment braveVpnConfirmDialogFragment =
            //         new BraveVpnConfirmDialogFragment();
            // braveVpnConfirmDialogFragment.setCancelable(false);
            // braveVpnConfirmDialogFragment.show(
            //         getFragmentManager(), "BraveVpnConfirmDialogFragment");
        }
    }
}
