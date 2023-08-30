/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity.ActivityType.GET_ENCRYPTION_PUBLIC_KEY_REQUEST;

import android.content.Intent;
import android.view.Window;
import android.view.WindowManager;

import androidx.annotation.MainThread;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.fragments.ApproveTxBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.AddSwitchChainNetworkFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.AddTokenFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.ConnectAccountFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.EncryptionKeyFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.SignMessageFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.SignTransactionFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.TransactionConfirmationListener;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;

import java.util.HashMap;
import java.util.Map;

public class BraveWalletDAppsActivity extends BraveWalletBaseActivity
        implements TransactionConfirmationListener,
                   AddSwitchChainNetworkFragment.AddSwitchRequestProcessListener {
    public static final String ACTIVITY_TYPE = "activityType";
    private static final String TAG = "BraveWalletDApps";
    private ApproveTxBottomSheetDialogFragment approveTxBottomSheetDialogFragment;
    private PendingTxHelper mPendingTxHelper;
    private Fragment mFragment;
    private WalletModel mWalletModel;

    public enum ActivityType {
        SIGN_MESSAGE(0),
        ADD_ETHEREUM_CHAIN(1),
        SWITCH_ETHEREUM_CHAIN(2),
        ADD_TOKEN(3),
        CONNECT_ACCOUNT(4),
        CONFIRM_TRANSACTION(5),
        DECRYPT_REQUEST(6),
        GET_ENCRYPTION_PUBLIC_KEY_REQUEST(7),
        SIGN_TRANSACTION(8),
        SIGN_ALL_TRANSACTIONS(9),
        FINISH(10);

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
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_brave_wallet_dapps);
        Intent intent = getIntent();
        mActivityType = ActivityType.valueOf(
                intent.getIntExtra("activityType", ActivityType.ADD_ETHEREUM_CHAIN.getValue()));
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            mWalletModel.getDappsModel().mProcessNextDAppsRequest.observe(this, activityType -> {
                if (activityType == null) return;
                switch (activityType) {
                    case GET_ENCRYPTION_PUBLIC_KEY_REQUEST:
                    case DECRYPT_REQUEST:
                        processPendingDappsRequest();
                        break;
                    case FINISH:
                        finish();
                        break;
                    default:
                        break;
                }
            });

        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "triggerLayoutInflation", e);
        }

        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        processPendingDappsRequest();
    }

    // TODO (pavi): use mProcessNextDAppsRequest and remove this callback
    @Override
    public void onAddRequestProcessed(boolean hasMoreRequests) {
        processPendingAddSwitchRequest(hasMoreRequests);
    }

    // TODO (pavi): use mProcessNextDAppsRequest and remove this callback
    @Override
    public void onSwitchRequestProcessed(boolean hasMoreRequests) {
        processPendingAddSwitchRequest(hasMoreRequests);
    }

    @Override
    public void onNextTransaction() {
        // TODO: show the info of next Transaction via the pendingTxHelper if available,
        //  otherwise dismiss activity
    }

    @Override
    public void onRejectAllTransactions() {
        for (TransactionInfo transactionInfo : mPendingTxHelper.getPendingTransactions()) {
            getTxService().rejectTransaction(
                    TransactionUtils.getCoinFromTxDataUnion(transactionInfo.txDataUnion),
                    transactionInfo.chainId, transactionInfo.id, success -> {
                        if (!success) {
                            Log.e(TAG, "Transaction failed " + transactionInfo.id);
                        }
                    });
        }
        mPendingTxHelper.destroy();
        finish();
    }

    @Override
    public void onCancel() {
        finish();
    }

    /*
     * Process the next request by reloading the AddSwitchChainNetworkFragment
     * which will load the next pending request
     */
    private void processPendingAddSwitchRequest(boolean hasMoreRequests) {
        if (hasMoreRequests) {
            processPendingDappsRequest();
        } else {
            finish();
        }
    }

    @MainThread
    private void processPendingDappsRequest() {
        ThreadUtils.assertOnUiThread();
        mFragment = null;
        if (mActivityType == ActivityType.SIGN_MESSAGE) {
            mFragment = new SignMessageFragment();
        } else if (mActivityType == ActivityType.ADD_TOKEN) {
            mFragment = new AddTokenFragment();
        } else if (mActivityType == ActivityType.CONFIRM_TRANSACTION) {
            mKeyringService.getAllAccounts(allAccounts -> {
                if (mPendingTxHelper != null) {
                    mPendingTxHelper.destroy();
                }
                mPendingTxHelper = new PendingTxHelper(
                        getTxService(), allAccounts.accounts, false, true, null);
                mPendingTxHelper.mHasNoPendingTxAfterProcessing.observe(this, hasNoPendingTx -> {
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
                                    mPendingTxHelper.getPendingTransactions(), transactionInfo,
                                    this);
                    approveTxBottomSheetDialogFragment.show(getSupportFragmentManager(),
                            ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
                    mPendingTxHelper.mTransactionInfoLd.observe(this, transactionInfos -> {
                        approveTxBottomSheetDialogFragment.setTxList(transactionInfos);
                    });
                });
                mPendingTxHelper.fetchTransactions(() -> {});
            });
        } else if (mActivityType == ActivityType.ADD_ETHEREUM_CHAIN
                || mActivityType == ActivityType.SWITCH_ETHEREUM_CHAIN) {
            mFragment = new AddSwitchChainNetworkFragment(mActivityType, this);
        } else if (mActivityType == ActivityType.SIGN_TRANSACTION
                || mActivityType == ActivityType.SIGN_ALL_TRANSACTIONS) {
            mFragment = SignTransactionFragment.newInstance(mActivityType);
        } else if (mActivityType == ActivityType.CONNECT_ACCOUNT) {
            mFragment = new ConnectAccountFragment();
        } else if (mActivityType == GET_ENCRYPTION_PUBLIC_KEY_REQUEST
                || mActivityType == ActivityType.DECRYPT_REQUEST) {
            mFragment = EncryptionKeyFragment.newInstance(mActivityType);
        }
        showCurrentFragment();
    }

    private void showCurrentFragment() {
        if (mFragment != null) {
            FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
            ft.replace(R.id.frame_layout, mFragment);
            ft.commit();
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        // need to clear the state for a fresh state next time
        // TODO (pavi): update the flow with dapps model
        // (under-development) and get rid of explicit clear state call
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.getWalletModel().getDappsModel().clearDappsState();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onDestroy " + e);
        }

        if (mPendingTxHelper != null) {
            mPendingTxHelper.destroy();
        }
    }
}
