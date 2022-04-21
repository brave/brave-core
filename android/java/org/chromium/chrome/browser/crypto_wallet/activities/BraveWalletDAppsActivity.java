/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK;
import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.ApproveTxBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.AddTokenFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.SignMessageFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.SwitchEthereumChainFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.TransactionConfirmationListener;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.settings.BraveWalletAddNetworksFragment;
import org.chromium.chrome.browser.settings.SettingsLauncherImpl;
import org.chromium.components.browser_ui.settings.SettingsLauncher;

import java.util.HashMap;
import java.util.Map;

public class BraveWalletDAppsActivity
        extends BraveWalletBaseActivity implements TransactionConfirmationListener {
    public static final String ACTIVITY_TYPE = "activityType";
    private static final String TAG = BraveWalletBaseActivity.class.getSimpleName();
    private ApproveTxBottomSheetDialogFragment approveTxBottomSheetDialogFragment;
    private PendingTxHelper mPendingTxHelper;

    public enum ActivityType {
        SIGN_MESSAGE(0),
        ADD_ETHEREUM_CHAIN(1),
        SWITCH_ETHEREUM_CHAIN(2),
        ADD_TOKEN(3),
        CONNECT_ACCOUNT(4),
        CONFIRM_TRANSACTION(5);

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
        } else if (mActivityType == ActivityType.CONFIRM_TRANSACTION) {
            getKeyringService().getKeyringInfo(
                    BraveWalletConstants.DEFAULT_KEYRING_ID, keyringInfo -> {
                        AccountInfo[] accountInfosTemp = keyringInfo.accountInfos;
                        if (accountInfosTemp == null) {
                            accountInfosTemp = new AccountInfo[] {};
                        }
                        mPendingTxHelper = new PendingTxHelper(
                                getTxService(), accountInfosTemp, false, null, true);
                        mPendingTxHelper.mHasNoPendingTxAfterProcessing.observe(
                                this, hasNoPendingTx -> {
                                    if (hasNoPendingTx) {
                                        finish();
                                    }
                                });
                        mPendingTxHelper.mSelectedPendingRequest.observe(this, transactionInfo -> {
                            if (transactionInfo == null
                                    || mPendingTxHelper.getPendingTransactions().size() == 0) {
                                return;
                            }
                            if (approveTxBottomSheetDialogFragment != null
                                    && approveTxBottomSheetDialogFragment.isVisible()) {
                                // TODO: instead of dismiss, show the details of new Tx once
                                //  onNextTransaction implementation is done
                                approveTxBottomSheetDialogFragment.dismiss();
                            }
                            approveTxBottomSheetDialogFragment =
                                    ApproveTxBottomSheetDialogFragment.newInstance(
                                            mPendingTxHelper.getPendingTransactions(),
                                            transactionInfo, "", this);
                            approveTxBottomSheetDialogFragment.show(getSupportFragmentManager(),
                                    ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
                            mPendingTxHelper.mTransactionInfoLd.observe(this, transactionInfos -> {
                                approveTxBottomSheetDialogFragment.setTxList(transactionInfos);
                            });
                        });
                        mPendingTxHelper.fetchTransactions(() -> {});
                    });
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

    @Override
    public void onNextTransaction() {
        // TODO: show the info of next Transaction via the pendingTxHelper if available,
        //  otherwise dismiss activity
    }

    @Override
    public void onRejectAllTransactions() {
        for (TransactionInfo transactionInfo : mPendingTxHelper.getPendingTransactions()) {
            getTxService().rejectTransaction(CoinType.ETH, transactionInfo.id, success -> {
                if (!success) {
                    Log.e(TAG, "Transaction failed " + transactionInfo.id);
                }
            });
        }
        finish();
    }

    @Override
    public void onCancel() {
        finish();
    }
}
