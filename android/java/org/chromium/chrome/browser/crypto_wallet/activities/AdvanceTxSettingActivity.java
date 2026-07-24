/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.widget.EditText;

import com.google.android.material.appbar.MaterialToolbar;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;

@NullMarked
public class AdvanceTxSettingActivity extends BraveWalletBaseActivity {
    private @Nullable String mTxId;
    private @Nullable String mChainId;
    private @Nullable String mNonce;

    /**
     * Creates an Intent object to open the advanced transaction settings activity.
     *
     * @param context Context used to create the Intent.
     * @param txId Id of the transaction to edit.
     * @param chainId Chain Id of the transaction to edit.
     * @param nonce Current transaction nonce, in hexadecimal format.
     * @return Intent object to open the advanced transaction settings activity.
     */
    public static Intent createIntent(
            final Context context, final String txId, final String chainId, final String nonce) {
        final Intent intent = new Intent(context, AdvanceTxSettingActivity.class);
        intent.putExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_ID, txId);
        intent.putExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_CHAIN_ID, chainId);
        intent.putExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_NONCE, nonce);

        return intent;
    }

    @Override
    protected void performPreInflationStartup() {
        super.performPreInflationStartup();

        // The launch Intent may be replaced by a plain MAIN Intent when the activity is recreated,
        // dropping its extras, so the saved instance state takes precedence over it.
        final Bundle savedInstanceState = getSavedInstanceState();
        if (savedInstanceState != null) {
            mTxId = savedInstanceState.getString(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_ID);
            mChainId =
                    savedInstanceState.getString(
                            WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_CHAIN_ID);
            mNonce =
                    savedInstanceState.getString(
                            WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_NONCE);
            return;
        }

        final Intent intent = getIntent();
        if (intent != null) {
            mTxId = intent.getStringExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_ID);
            mChainId = intent.getStringExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_CHAIN_ID);
            mNonce = intent.getStringExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_NONCE);
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putString(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_ID, mTxId);
        outState.putString(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_CHAIN_ID, mChainId);
        outState.putString(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_NONCE, mNonce);
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_advance_tx_setting);

        final MaterialToolbar toolbar = findViewById(R.id.toolbar);
        toolbar.setTitle(R.string.brave_wallet_advanced_settings);
        setSupportActionBar(toolbar);

        final EditText etCustomNonce = findViewById(R.id.activity_advance_setting_et_nonce);
        etCustomNonce.setText(mNonce == null ? "" : Utils.hexToIntString(mNonce));

        findViewById(R.id.activity_advance_setting_btn_save)
                .setOnClickListener(
                        v -> {
                            final String newNonce = etCustomNonce.getText().toString();
                            setNonceForUnapprovedTransaction(
                                    TextUtils.isEmpty(newNonce) ? newNonce : Utils.toHex(newNonce));
                        });
        onInitialLayoutInflationComplete();
    }

    private void setNonceForUnapprovedTransaction(final String newNonce) {
        if (mTxId == null || mChainId == null) {
            return;
        }

        getEthTxManagerProxy()
                .setNonceForUnapprovedTransaction(
                        mChainId,
                        mTxId,
                        newNonce,
                        isSet -> {
                            if (isSet && !isActivityFinishingOrDestroyed()) {
                                Intent result =
                                        new Intent()
                                                .putExtra(
                                                        WalletConstants
                                                                .ADVANCE_TX_SETTING_INTENT_RESULT_NONCE,
                                                        newNonce);
                                setResult(Activity.RESULT_OK, result);
                                finish();
                            }
                        });
    }
}
