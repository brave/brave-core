/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.view.Window;

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;

import org.chromium.base.Log;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.fragments.ApproveTxBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.WalletFragmentCallback;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.AddSwitchChainNetworkFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.AddTokenFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.ConnectAccountFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.EncryptionKeyFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.SignMessageErrorFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.SignMessageFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.SignSolTransactionsFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.SiweMessageFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.TransactionListener;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;
import org.chromium.chrome.browser.init.ActivityProfileProvider;
import org.chromium.chrome.browser.profiles.ProfileProvider;

import java.util.ArrayList;
import java.util.List;

/** Base activity for all DApps-related activities */
public class BraveWalletDAppsActivity extends BraveWalletBaseActivity
        implements TransactionListener,
                WalletFragmentCallback,
                AddSwitchChainNetworkFragment.AddSwitchRequestProcessListener {
    public static final String EXTRA_ACTIVITY_TYPE =
            "org.chromium.chrome.browser.crypto_wallet.activities.ACTIVITY_TYPE";

    private static final String TAG = "BraveWalletDApps";
    private final List<TransactionInfo> mPendingTransactions = new ArrayList<>();
    @Nullable private TransactionInfo mCurrentTransaction;
    private ApproveTxBottomSheetDialogFragment mApproveTxBottomSheetDialogFragment;
    private PendingTxHelper mPendingTxHelper;
    private Fragment mFragment;
    private WalletModel mWalletModel;
    @MonotonicNonNull private ActivityType mActivityType;

    public enum ActivityType {
        SIGN_MESSAGE,
        ADD_ETHEREUM_CHAIN,
        SWITCH_ETHEREUM_CHAIN,
        ADD_TOKEN,
        CONNECT_ACCOUNT,
        CONFIRM_TRANSACTION,
        DECRYPT_REQUEST,
        GET_ENCRYPTION_PUBLIC_KEY_REQUEST,
        SIGN_TRANSACTION_DEPRECATED,
        SIGN_SOL_TRANSACTIONS,
        SIWE_MESSAGE,
        SIGN_MESSAGE_ERROR;
    }

    @Override
    protected @Nullable Bundle transformSavedInstanceStateForOnCreate(
            @Nullable Bundle savedInstanceState) {
        // The DApps fragments depend on the WalletModel, which only becomes available
        // after native initialization (finishNativeInitialization). On a configuration
        // change such as rotation, the FragmentManager would otherwise restore these
        // fragments before the model is ready and crash in
        // WalletBottomSheetDialogFragment.onAttach. Drop the saved fragment/view state so
        // the activity rebuilds its fragment from the pending request once the model is
        // initialized. The activity itself is fully reconstructed from its intent and the
        // wallet's pending requests, so nothing else needs to be restored.
        return null;
    }

    @Override
    protected void onPreCreate() {
        super.onPreCreate();
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        final ActivityType activityType = getActivityType(getIntent());
        mActivityType = activityType != null ? activityType : ActivityType.ADD_ETHEREUM_CHAIN;
        // Closing the panel any other way than onAllRequestsHandled() means it was dismissed.
        setResult(RESULT_CANCELED, createResult());
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_brave_wallet_dapps);
        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "finishNativeInitialization", e);
        }

        processPendingDappsRequest();
    }

    @Override
    public void onAddRequestProcessed(boolean hasMoreRequests) {
        processPendingAddSwitchRequest(hasMoreRequests);
    }

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
            getTxService()
                    .rejectTransaction(
                            TransactionUtils.getCoinFromTxDataUnion(transactionInfo.txDataUnion),
                            transactionInfo.chainId,
                            transactionInfo.id,
                            success -> {
                                if (!success) {
                                    Log.e(TAG, "Transaction failed " + transactionInfo.id);
                                }
                            });
        }
        mPendingTxHelper.destroy();
        onAllRequestsHandled();
    }

    @Override
    public void onCancel() {
        finish();
    }

    @Override
    public WalletModel getWalletModel() {
        return mWalletModel;
    }

    @Override
    public void showPanel(final ActivityType activityType) {
        mActivityType = activityType;
        processPendingDappsRequest();
    }

    @Override
    public void onAllRequestsHandled() {
        setResult(RESULT_OK, createResult());
        finish();
    }

    @Override
    public List<TransactionInfo> getPendingTransactions() {
        return mPendingTransactions;
    }

    @Override
    @Nullable
    public TransactionInfo getCurrentTransaction() {
        return mCurrentTransaction;
    }

    /*
     * Process the next request by reloading the AddSwitchChainNetworkFragment
     * which will load the next pending request
     */
    private void processPendingAddSwitchRequest(boolean hasMoreRequests) {
        if (hasMoreRequests) {
            processPendingDappsRequest();
        } else {
            onAllRequestsHandled();
        }
    }

    private void processPendingDappsRequest() {
        mFragment = null;
        switch (mActivityType) {
            case ActivityType.SIGN_MESSAGE:
                mFragment = new SignMessageFragment();
                break;
            case ActivityType.SIWE_MESSAGE:
                mFragment = new SiweMessageFragment();
                break;
            case ActivityType.SIGN_MESSAGE_ERROR:
                mFragment = new SignMessageErrorFragment();
                break;
            case ActivityType.ADD_TOKEN:
                mFragment = new AddTokenFragment();
                break;
            case ActivityType.CONFIRM_TRANSACTION:
                mKeyringService.getAllAccounts(
                        allAccounts -> {
                            if (mPendingTxHelper != null) {
                                mPendingTxHelper.destroy();
                            }
                            mPendingTxHelper =
                                    new PendingTxHelper(
                                            getTxService(), allAccounts.accounts, false, true);
                            mPendingTxHelper.mHasNoPendingTxAfterProcessing.observe(
                                    this,
                                    hasNoPendingTx -> {
                                        if (hasNoPendingTx) {
                                            onAllRequestsHandled();
                                        }
                                    });
                            mPendingTxHelper.mSelectedPendingRequest.observe(
                                    this,
                                    transactionInfo -> {
                                        if (transactionInfo == null
                                                || mPendingTxHelper
                                                        .getPendingTransactions()
                                                        .isEmpty()) {
                                            return;
                                        }
                                        if (mApproveTxBottomSheetDialogFragment != null
                                                && mApproveTxBottomSheetDialogFragment
                                                        .isVisible()) {
                                            // TODO: instead of dismiss, show the details of new Tx
                                            // once onNextTransaction implementation is done
                                            mApproveTxBottomSheetDialogFragment.dismiss();
                                        }
                                        mCurrentTransaction = transactionInfo;
                                        mPendingTransactions.clear();
                                        mPendingTransactions.addAll(
                                                mPendingTxHelper.getPendingTransactions());
                                        mApproveTxBottomSheetDialogFragment =
                                                new ApproveTxBottomSheetDialogFragment();
                                        mApproveTxBottomSheetDialogFragment.show(
                                                getSupportFragmentManager());
                                        mPendingTxHelper.mTransactionInfoLd.observe(
                                                this,
                                                transactionInfos -> {
                                                    mApproveTxBottomSheetDialogFragment.setTxList(
                                                            transactionInfos);
                                                });
                                    });
                            mPendingTxHelper.fetchTransactions();
                        });
                break;
            case ActivityType.ADD_ETHEREUM_CHAIN:
            case ActivityType.SWITCH_ETHEREUM_CHAIN:
                mFragment = AddSwitchChainNetworkFragment.newInstance(mActivityType);
                break;
            case ActivityType.SIGN_SOL_TRANSACTIONS:
                mFragment = new SignSolTransactionsFragment();
                break;
            case ActivityType.CONNECT_ACCOUNT:
                mFragment = new ConnectAccountFragment();
                break;
            case ActivityType.GET_ENCRYPTION_PUBLIC_KEY_REQUEST:
            case ActivityType.DECRYPT_REQUEST:
                mFragment = EncryptionKeyFragment.newInstance(mActivityType);
                break;
            default:
                break;
        }
        showCurrentFragment();
    }

    private void showCurrentFragment() {
        if (mFragment != null) {
            FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
            ft.replace(R.id.fragment_container, mFragment);
            ft.commit();
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mPendingTxHelper != null) {
            mPendingTxHelper.destroy();
        }
    }

    @Override
    protected OneshotSupplier<ProfileProvider> createProfileProvider() {
        return new ActivityProfileProvider(getLifecycleDispatcher());
    }

    /** Returns the kind of panel {@code intent} starts, or reports back in its result. */
    public static @Nullable ActivityType getActivityType(@Nullable final Intent intent) {
        if (intent == null) {
            return null;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            return intent.getSerializableExtra(EXTRA_ACTIVITY_TYPE, ActivityType.class);
        }
        return (ActivityType) intent.getSerializableExtra(EXTRA_ACTIVITY_TYPE);
    }

    private Intent createResult() {
        return new Intent().putExtra(EXTRA_ACTIVITY_TYPE, mActivityType);
    }

    public static Intent getIntent(final Context context, final ActivityType activityType) {
        final Intent braveWalletIntent = new Intent(context, BraveWalletDAppsActivity.class);
        braveWalletIntent.putExtra(EXTRA_ACTIVITY_TYPE, activityType);
        braveWalletIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        braveWalletIntent.setAction(Intent.ACTION_VIEW);
        return braveWalletIntent;
    }
}
