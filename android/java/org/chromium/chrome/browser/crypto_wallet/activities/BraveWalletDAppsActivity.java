/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Intent;

import androidx.fragment.app.FragmentTransaction;

import org.chromium.chrome.R;

import java.util.HashMap;
import java.util.Map;

public class BraveWalletDAppsActivity extends BraveWalletBaseActivity {
    public static final String ACTIVITY_TYPE = "activityType";

    public enum ActivityType {
        SIGN_MESSAGE(0),
        ADD_ETHEREUM_CHAIN(1),
        SWITCH_ETHEREUM_CHAIN(2),
        ADD_TOKEN(3);

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
        FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
        // if (mActivityType == ActivityType.SIGN_MESSAGE) {
        //     ft.replace(R.id.frame_layout, new SignMessageFragment());
        // } else if (mActivityType == ActivityType.ADD_ETHEREUM_CHAIN) {
        //     ft.replace(R.id.frame_layout, new AddEthereumChainFragment());
        // } else if (mActivityType == ActivityType.SWITCH_ETHEREUM_CHAIN) {
        //     ft.replace(R.id.frame_layout, new SwitchEthereumChainFragment());
        // } else if (mActivityType == ActivityType.ADD_TOKEN) {
        //     ft.replace(R.id.frame_layout, new AddTokenFragmentFragment());
        // }
        ft.commit();
    }
}
