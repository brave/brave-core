/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK;
import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID;

import android.content.Intent;
import android.os.Bundle;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.AddTokenFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.SignMessageFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.SwitchEthereumChainFragment;
import org.chromium.chrome.browser.settings.BraveWalletAddNetworksFragment;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.components.browser_ui.settings.SettingsLauncher;

import java.util.HashMap;
import java.util.Map;

public class BraveWalletDAppsActivity extends BraveWalletBaseActivity {
    public static final String ACTIVITY_TYPE = "activityType";

    public enum ActivityType {
        SIGN_MESSAGE(0),
        ADD_ETHEREUM_CHAIN(1),
        SWITCH_ETHEREUM_CHAIN(2),
        ADD_TOKEN(3),
        CONNECT_ACCOUNT(4);

        private int value;
        private static Map map = new HashMap<>();

        private ActivityType(int value) {
            this.value = value;
        }

        static {
            for (ActivityType activityType : ActivityType.values()) {
                map.put(activityType.value, activityType);
            }
        }

        public static ActivityType valueOf(int activityType) {
            return (ActivityType) map.get(activityType);
        }

        public int getValue() {
            return value;
        }
    }

    private ActivityType mActivityType;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_brave_wallet_dapps);
        Intent intent = getIntent();
        mActivityType = ActivityType.valueOf(
                intent.getIntExtra("activityType", ActivityType.ADD_ETHEREUM_CHAIN.getValue()));
        Fragment fragment = null;
        if (mActivityType == ActivityType.SIGN_MESSAGE) {
            fragment = new SignMessageFragment();
        } else if (mActivityType == ActivityType.SWITCH_ETHEREUM_CHAIN) {
            fragment = new SwitchEthereumChainFragment();
        } else if (mActivityType == ActivityType.ADD_TOKEN) {
            fragment = new AddTokenFragment();
        }

        if (fragment != null) {
            FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
            ft.replace(R.id.frame_layout, fragment);
            ft.commit();
        }
        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        if (mActivityType == ActivityType.ADD_ETHEREUM_CHAIN) {
            addEthereumChain();
            finish();
        }
    }

    private void addEthereumChain() {
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putString(ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID, "");
        fragmentArgs.putBoolean(ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK, false);
        SettingsLauncher settingsLauncher = new SettingsLauncherImpl();

        settingsLauncher.launchSettingsActivity(
                this, BraveWalletAddNetworksFragment.class, fragmentArgs);
    }
}
