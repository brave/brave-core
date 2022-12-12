/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser;

import android.content.Intent;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsExternalWallet;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveWalletProvider;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.ledger.mojom.WalletStatus;

public class BraveRewardsUserWalletActivity extends AsyncInitializationActivity {
    private static final String TAG = "BraveRewards";
    public static final String DISCONNECT_WALLET_URL = "brave://rewards/#disconnect-wallet";
    public static final int UNDEFINED_WALLET_STATUS = -1;

    private String walletType = BraveRewardsNativeWorker.getInstance().getExternalWalletType();

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.user_wallet_activity);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        ActionBar ab = getSupportActionBar();
        ab.setDisplayHomeAsUpEnabled(true);
        ab.setDisplayShowTitleEnabled(false);

        SetUIControls();

        onInitialLayoutInflationComplete();
    }

    private void SetUIControls() {
        Intent intent = getIntent();
        final int status =
                intent.getIntExtra(BraveRewardsExternalWallet.STATUS, UNDEFINED_WALLET_STATUS);
        TextView txtUserId = (TextView) findViewById(R.id.user_id);
        TextView txtUserStatus = (TextView) findViewById(R.id.user_status);
        Button btnGotoProvider = (Button) findViewById(R.id.user_wallet_go_to_provider);
        btnGotoProvider.setText(
                String.format(getResources().getString(R.string.user_wallet_goto_provider),
                        getWalletString(walletType)));

        switch (status) {
            case WalletStatus.CONNECTED:
                txtUserStatus.setText(BraveRewardsExternalWallet.WalletStatusToString(status));
                break;
            case UNDEFINED_WALLET_STATUS:
                finish();
                break;
        }

        SetBtnOpenUrlClickHandler(
                btnGotoProvider, intent.getStringExtra(BraveRewardsExternalWallet.ACCOUNT_URL));

        String userId = intent.getStringExtra(BraveRewardsExternalWallet.USER_NAME);
        txtUserId.setText(userId);
        txtUserId.setCompoundDrawablesWithIntrinsicBounds(getWalletIcon(walletType), 0, 0, 0);
    }

    private int getWalletIcon(String walletType) {
        if (walletType.equals(BraveWalletProvider.UPHOLD)) {
            return R.drawable.uphold_green;
        } else if (walletType.equals(BraveWalletProvider.GEMINI)) {
            return R.drawable.ic_gemini_logo_cyan;
        } else {
            return R.drawable.ic_logo_bitflyer_colored;
        }
    }

    private String getWalletString(String walletType) {
        if (walletType.equals(BraveWalletProvider.UPHOLD)) {
            return getResources().getString(R.string.uphold);
        } else if (walletType.equals(BraveWalletProvider.GEMINI)) {
            return getResources().getString(R.string.gemini);
        } else {
            return getResources().getString(R.string.bitflyer);
        }
    }

    private void SetBtnOpenUrlClickHandler(Button btn, String url) {
        btn.setOnClickListener((View v) -> {
            Intent intent = new Intent();
            intent.putExtra(BraveActivity.OPEN_URL, url);
            setResult(RESULT_OK, intent);
            finish();
        });
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            finish();
        }
        return super.onOptionsItemSelected(item);
    }
}
