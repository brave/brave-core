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

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsExternalWallet;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveWalletProvider;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.ledger.mojom.WalletStatus;

public class BraveRewardsUserWalletActivity
        extends AsyncInitializationActivity implements View.OnClickListener {
    private static final String TAG = "BraveRewards";
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

        setUIControls();

        onInitialLayoutInflationComplete();
    }

    private void setUIControls() {
        Intent intent = getIntent();
        final int status =
                intent.getIntExtra(BraveRewardsExternalWallet.STATUS, UNDEFINED_WALLET_STATUS);
        TextView txtUserId = (TextView) findViewById(R.id.user_id);
        TextView txtUserStatus = (TextView) findViewById(R.id.user_status);
        Button btnGotoProvider = (Button) findViewById(R.id.provider_action);
        btnGotoProvider.setOnClickListener(this);
        String providerText = (status == WalletStatus.CONNECTED)
                ? String.format(getResources().getString(R.string.user_wallet_goto_provider),
                        getWalletString(walletType))
                : String.format(getResources().getString(R.string.login_provider),
                        getWalletString(walletType));
        btnGotoProvider.setText(providerText);

        switch (status) {
            case WalletStatus.CONNECTED:
                txtUserStatus.setText(BraveRewardsExternalWallet.WalletStatusToString(status));
                break;
            case WalletStatus.LOGGED_OUT:
                txtUserStatus.setText(BraveRewardsExternalWallet.WalletStatusToString(status));
                break;
            case UNDEFINED_WALLET_STATUS:
                finish();
                break;
        }

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

    @Override
    public void onClick(@NonNull View view) {
        if (view.getId() == R.id.provider_action) {
            if (getIntent() != null) {
                int walletStatus = getIntent().getIntExtra(
                        BraveRewardsExternalWallet.STATUS, UNDEFINED_WALLET_STATUS);
                if (walletStatus == WalletStatus.CONNECTED) {
                    if (getIntent().getStringExtra(BraveRewardsExternalWallet.ACCOUNT_URL)
                            != null) {
                        Intent intent = new Intent();
                        intent.putExtra(BraveActivity.OPEN_URL,
                                getIntent().getStringExtra(BraveRewardsExternalWallet.ACCOUNT_URL));
                        setResult(RESULT_OK, intent);
                    }
                } else if (walletStatus == WalletStatus.LOGGED_OUT) {
                    Intent intent = new Intent();
                    intent.putExtra(BraveActivity.OPEN_URL,
                            getIntent().getStringExtra(BraveRewardsExternalWallet.LOGIN_URL));
                    setResult(RESULT_OK, intent);
                }
                finish();
            }
        }
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
