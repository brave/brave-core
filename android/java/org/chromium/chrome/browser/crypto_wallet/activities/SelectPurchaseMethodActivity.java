/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.chrome.browser.crypto_wallet.util.Utils.warnWhenError;

import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;
import android.widget.Button;

import androidx.appcompat.widget.Toolbar;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.util.TabUtils;

public class SelectPurchaseMethodActivity extends BraveWalletBaseActivity {
    private static final String RAMP_NETWORK_URL = "rampNetworkUrl";

    private String mRampNetworkUrl;
    private Button mRampButton;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_select_purchase_method);

        Intent intent = getIntent();
        if (intent != null) {
            mRampNetworkUrl = intent.getStringExtra(RAMP_NETWORK_URL);
        }
        
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mRampButton = findViewById(R.id.purchase_method_btn_ramp);

        if (!TextUtils.isEmpty(mRampNetworkUrl)) {
            mRampButton.setOnClickListener(v -> {
                TabUtils.openUrlInNewTab(false, mRampNetworkUrl);
                TabUtils.bringChromeTabbedActivityToTheTop(this);
            });
        }

        onInitialLayoutInflationComplete();
    }

    public static Intent getIntent(Context context, String rampNetworkUrl) {
        Intent intent = new Intent(context, SelectPurchaseMethodActivity.class);
        intent.putExtra(RAMP_NETWORK_URL, rampNetworkUrl);
        intent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        return intent;
    }
}
