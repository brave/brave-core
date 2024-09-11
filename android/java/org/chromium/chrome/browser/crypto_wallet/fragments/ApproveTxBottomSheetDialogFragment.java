/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.webkit.URLUtil;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.tabs.TabLayout;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.ProviderErrorUnion;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.ApproveTxFragmentPageAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.TransactionConfirmationListener;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.ParsedTransaction;
import org.chromium.chrome.browser.crypto_wallet.util.SolanaTransactionsGasHelper;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ApproveTxBottomSheetDialogFragment extends WalletBottomSheetDialogFragment {
    private static final String TAG = "ApproveTx";

    public static final String TAG_FRAGMENT = ApproveTxBottomSheetDialogFragment.class.getName();

    private TransactionInfo mTxInfo;
    private boolean mRejected;
    private boolean mApproved;
    private ApprovedTxObserver mApprovedTxObserver;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private TransactionConfirmationListener mTransactionConfirmationListener;
    private List<TransactionInfo> mTransactionInfos;
    private Button mRejectAllTx;
    private int mCoinType;
    private long mSolanaEstimatedTxFee;
    private WalletModel mWalletModel;
    private NetworkInfo mTxNetwork;

    public static ApproveTxBottomSheetDialogFragment newInstance(
            List<TransactionInfo> transactionInfos,
            TransactionInfo txInfo,
            TransactionConfirmationListener listener) {
        return new ApproveTxBottomSheetDialogFragment(transactionInfos, txInfo, listener);
    }

    public static ApproveTxBottomSheetDialogFragment newInstance(TransactionInfo txInfo) {
        List<TransactionInfo> infos = new ArrayList<>();
        infos.add(txInfo);
        return newInstance(infos, txInfo, null);
    }

    private ApproveTxBottomSheetDialogFragment(TransactionInfo txInfo) {
        mTxInfo = txInfo;
        mRejected = false;
        mApproved = false;
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        // TODO (Wengling): To support other networks, all hard-coded chainSymbol, etc. need to be
        // get from current network instead.
        mTransactionInfos = Collections.emptyList();
        mSolanaEstimatedTxFee = 0;
    }

    ApproveTxBottomSheetDialogFragment(
            List<TransactionInfo> transactionInfos,
            TransactionInfo txInfo,
            @Nullable TransactionConfirmationListener transactionConfirmationListener) {
        this(txInfo);
        mTransactionInfos = transactionInfos;
        mTransactionConfirmationListener = transactionConfirmationListener;
    }

    public void setApprovedTxObserver(ApprovedTxObserver approvedTxObserver) {
        mApprovedTxObserver = approvedTxObserver;
    }

    // TODO: Make these into an interface so they can be shared between fragments and activities
    private AssetRatioService getAssetRatioService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getAssetRatioService();
        }
        return null;
    }

    private TxService getTxService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getTxService();
        }
        return null;
    }

    private JsonRpcService getJsonRpcService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getJsonRpcService();
        }
        return null;
    }

    private BlockchainRegistry getBlockchainRegistry() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getBlockchainRegistry();
        }
        return null;
    }

    private BraveWalletService getBraveWalletService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getBraveWalletService();
        }
        return null;
    }

    private KeyringService getKeyringService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getKeyringService();
        }
        return null;
    }

    @Override
    public void show(FragmentManager manager, String tag) {
        try {
            ApproveTxBottomSheetDialogFragment fragment =
                    (ApproveTxBottomSheetDialogFragment)
                            manager.findFragmentByTag(
                                    ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e("ApproveTxBottomSheetDialogFragment", e.getMessage());
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Dialog dialog = super.onCreateDialog(savedInstanceState);
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            registerKeyringObserver(mWalletModel.getKeyringModel());
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreateDialog ", e);
        }
        return dialog;
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        super.onDismiss(dialog);
        if (mApprovedTxObserver != null) {
            if (mRejected || mApproved) {
                // TODO(pav): 28/07/22 rename to callback or delegate, it's not an observer
                mApprovedTxObserver.onTxApprovedRejected(mApproved, mTxInfo.id);
            } else {
                mApprovedTxObserver.onTxPending(mTxInfo.id);
            }
        }
        if (mTransactionConfirmationListener != null) {
            mTransactionConfirmationListener.onDismiss();
        }
    }

    @Override
    public void setupDialog(@NonNull Dialog dialog, int style) {
        super.setupDialog(dialog, style);
        @SuppressLint("InflateParams")
        final View view =
                LayoutInflater.from(getContext()).inflate(R.layout.approve_tx_bottom_sheet, null);
        dialog.setContentView(view);
        ViewParent parent = view.getParent();
        ((View) parent).getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
        JsonRpcService jsonRpcService = getJsonRpcService();
        KeyringService keyringService = getKeyringService();
        assert jsonRpcService != null && keyringService != null;

        TextView networkName = view.findViewById(R.id.network_name);
        TextView txType = view.findViewById(R.id.tx_type);
        if (mTxInfo.txType
                == TransactionType
                        .SOLANA_SPL_TOKEN_TRANSFER_WITH_ASSOCIATED_TOKEN_ACCOUNT_CREATION) {
            TextView associatedSplTokenInfo =
                    view.findViewById(R.id.tv_approve_dialog_additional_details);
            associatedSplTokenInfo.setVisibility(View.VISIBLE);
            Spanned associatedSPLTokenAccountInfo =
                    Utils.createSpanForSurroundedPhrase(
                            getContext(),
                            R.string.brave_wallet_confirm_transaction_account_creation_fee,
                            (v) -> {
                                TabUtils.openUrlInNewTab(false, Utils.BRAVE_SUPPORT_URL);
                                TabUtils.bringChromeTabbedActivityToTheTop(getActivity());
                            });
            associatedSplTokenInfo.setMovementMethod(LinkMovementMethod.getInstance());
            associatedSplTokenInfo.setText(associatedSPLTokenAccountInfo);
        }
        mCoinType = TransactionUtils.getCoinFromTxDataUnion(mTxInfo.txDataUnion);
        if (JavaUtils.anyNull(mWalletModel)) return;
        mTxNetwork = mWalletModel.getNetworkModel().getNetwork(mTxInfo.chainId);
        networkName.setText(mTxNetwork.chainName);
        keyringService.getAllAccounts(
                allAccounts -> {
                    AccountInfo[] accounts =
                            AssetUtils.filterAccountsByNetwork(
                                    allAccounts.accounts, mTxNetwork.coin, mTxNetwork.chainId);

                    AccountInfo txAccountInfo = Utils.findAccount(accounts, mTxInfo.fromAccountId);
                    if (txAccountInfo == null) {
                        return;
                    }

                    ImageView icon = (ImageView) view.findViewById(R.id.account_picture);
                    Utils.setBlockiesBitmapResourceFromAccount(
                            mExecutor, mHandler, icon, txAccountInfo, true);

                    // First fill in data that does not require remote queries
                    TokenUtils.getAllTokensFiltered(
                            getBraveWalletService(),
                            getBlockchainRegistry(),
                            mTxNetwork,
                            TokenUtils.TokenType.ALL,
                            tokenList -> {
                                SolanaTransactionsGasHelper solanaTransactionsGasHelper =
                                        new SolanaTransactionsGasHelper(
                                                (BraveWalletBaseActivity) getActivity(),
                                                new TransactionInfo[] {mTxInfo});
                                solanaTransactionsGasHelper.maybeGetSolanaGasEstimations(
                                        () -> {
                                            HashMap<String, Long> perTxFee =
                                                    solanaTransactionsGasHelper.getPerTxFee();
                                            if (perTxFee.get(mTxInfo.id) != null) {
                                                mSolanaEstimatedTxFee = perTxFee.get(mTxInfo.id);
                                            }
                                            if (!canUpdateUi()) return;
                                            ParsedTransaction parsedTx =
                                                    fillAssetDependentControls(
                                                            view,
                                                            mTxNetwork,
                                                            txAccountInfo,
                                                            accounts,
                                                            new HashMap<String, Double>(),
                                                            tokenList,
                                                            new HashMap<String, Double>(),
                                                            new HashMap<
                                                                    String,
                                                                    HashMap<String, Double>>(),
                                                            mSolanaEstimatedTxFee);

                                            // Get tokens involved in this transaction
                                            List<BlockchainToken> tokens = new ArrayList<>();
                                            tokens.add(
                                                    Utils.makeNetworkAsset(
                                                            mTxNetwork)); // Always add native asset
                                            if (parsedTx.getIsSwap()) {
                                                tokens.add(parsedTx.getSellToken());
                                                tokens.add(parsedTx.getBuyToken());
                                            } else if (parsedTx.getToken() != null)
                                                tokens.add(parsedTx.getToken());
                                            BlockchainToken[] filterByTokens =
                                                    tokens.toArray(new BlockchainToken[0]);

                                            fetchTxBalanceAndUpdateUi(
                                                    view,
                                                    mTxNetwork,
                                                    txAccountInfo,
                                                    accounts,
                                                    filterByTokens);
                                        });
                            });
                });
        Button reject = view.findViewById(R.id.reject);
        reject.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        rejectTransaction(true);
                    }
                });
        Button approve = view.findViewById(R.id.approve);
        approve.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        approveTransaction();
                    }
                });
        if (mTransactionInfos.size() > 1) {
            // TODO: next button is not functional. Update next button text based on position in
            //  mTransactionInfos list.
            //  Refactor Approve pendingTxHelper code to update the mSelectedPendingRequest Tx
            //  with the next Transaction.
            Button next = view.findViewById(R.id.btn_next_tx);
            next.setVisibility(View.VISIBLE);
            next.setOnClickListener(
                    v -> {
                        if (mTransactionConfirmationListener != null) {
                            mTransactionConfirmationListener.onNextTransaction();
                        }
                    });
            mRejectAllTx = view.findViewById(R.id.btn_reject_transactions);
            mRejectAllTx.setVisibility(View.VISIBLE);
            mRejectAllTx.setOnClickListener(
                    v -> {
                        if (mTransactionConfirmationListener != null) {
                            mTransactionConfirmationListener.onRejectAllTransactions();
                            dismiss();
                        }
                    });
            refreshListContentUi();
        }

        if (mTxInfo.originInfo != null && URLUtil.isValidUrl(mTxInfo.originInfo.originSpec)) {
            TextView domain = view.findViewById(R.id.domain);
            domain.setVisibility(View.VISIBLE);
            domain.setText(Utils.geteTldSpanned(mTxInfo.originInfo));
        }
    }

    public void setTxList(List<TransactionInfo> transactionInfos) {
        mTransactionInfos = transactionInfos;
        if (isVisible()) {
            refreshListContentUi();
        }
    }

    private void fetchTxBalanceAndUpdateUi(
            View view,
            NetworkInfo txNetwork,
            AccountInfo txAccountInfo,
            AccountInfo[] accounts,
            BlockchainToken[] filterByTokens) {
        if (mWalletModel == null) return;
        LiveDataUtil.observeOnce(
                mWalletModel.getCryptoModel().getNetworkModel().mCryptoNetworks,
                allNetworks -> {
                    Utils.getTxExtraInfo(
                            new WeakReference<>((BraveWalletBaseActivity) getActivity()),
                            TokenUtils.TokenType.ALL,
                            allNetworks,
                            txNetwork,
                            accounts,
                            filterByTokens,
                            false,
                            (assetPrices,
                                    fullTokenList,
                                    nativeAssetsBalances,
                                    blockchainTokensBalances) -> {
                                if (!canUpdateUi()) return;
                                fillAssetDependentControls(
                                        view,
                                        txNetwork,
                                        txAccountInfo,
                                        accounts,
                                        assetPrices,
                                        fullTokenList,
                                        nativeAssetsBalances,
                                        blockchainTokensBalances,
                                        mSolanaEstimatedTxFee);
                            });
                });
    }

    private void refreshListContentUi() {
        mRejectAllTx.setText(
                getString(
                        R.string.brave_wallet_queue_reject_all,
                        String.valueOf(mTransactionInfos.size())));
    }

    private ParsedTransaction fillAssetDependentControls(
            View view,
            NetworkInfo txNetwork,
            AccountInfo txAccountInfo,
            AccountInfo[] accounts,
            HashMap<String, Double> assetPrices,
            BlockchainToken[] fullTokenList,
            HashMap<String, Double> nativeAssetsBalances,
            HashMap<String, HashMap<String, Double>> blockchainTokensBalances,
            long solanaEstimatedTxFee) {
        ParsedTransaction parsedTx =
                ParsedTransaction.parseTransaction(
                        mTxInfo,
                        txNetwork,
                        accounts,
                        assetPrices,
                        solanaEstimatedTxFee,
                        fullTokenList,
                        nativeAssetsBalances,
                        blockchainTokensBalances);
        TextView txType = view.findViewById(R.id.tx_type);
        if (parsedTx.getType() == TransactionType.ERC20_APPROVE) {
            txType.setText(
                    String.format(
                            getResources().getString(R.string.activate_erc20),
                            parsedTx.getSymbol()));
        } else if (parsedTx.getIsSwap()) {
            txType.setText(getResources().getString(R.string.swap));
        } else if (parsedTx.isSolanaDappTransaction) {
            txType.setText(R.string.brave_wallet_approve_transaction);
        } else {
            txType.setText(getResources().getString(R.string.send));
        }

        TextView amountFiat = view.findViewById(R.id.amount_fiat);
        TextView amountAsset = view.findViewById(R.id.amount_asset);
        if (parsedTx.isSolanaDappTransaction) {
            AndroidUtils.gone(amountFiat, amountAsset);
        } else {
            amountFiat.setText(
                    String.format(
                            getResources().getString(R.string.crypto_wallet_amount_fiat),
                            String.format(Locale.getDefault(), "%.2f", parsedTx.getFiatTotal())));
            String amountText =
                    String.format(
                            getResources().getString(R.string.crypto_wallet_amount_asset),
                            parsedTx.formatValueToDisplay(),
                            parsedTx.getSymbol());

            if (mTxInfo.txType == TransactionType.ERC721_TRANSFER_FROM
                    || mTxInfo.txType == TransactionType.ERC721_SAFE_TRANSFER_FROM) {
                amountText = Utils.tokenToString(parsedTx.getErc721BlockchainToken());
                amountFiat.setVisibility(View.GONE); // Display NFT values in the future
            }
            amountAsset.setText(amountText);
        }

        TextView fromTo = view.findViewById(R.id.from_to);
        if (parsedTx.getSender() != null && !parsedTx.getSender().equals(parsedTx.getRecipient())) {
            String recipient =
                    TextUtils.isEmpty(parsedTx.getRecipient()) ? "..." : parsedTx.getRecipient();
            fromTo.setText(
                    String.format(
                            getResources().getString(R.string.crypto_wallet_from_to),
                            txAccountInfo.name,
                            parsedTx.getSender(),
                            "->",
                            recipient));
        } else {
            fromTo.setText(
                    String.format(
                            getResources().getString(R.string.crypto_wallet_from_to),
                            txAccountInfo.name,
                            parsedTx.getSender(),
                            "",
                            ""));
        }
        setupPager(
                view,
                txNetwork,
                accounts,
                assetPrices,
                fullTokenList,
                nativeAssetsBalances,
                blockchainTokensBalances);
        return parsedTx;
    }

    private void setupPager(
            View view,
            NetworkInfo txNetwork,
            AccountInfo[] accounts,
            HashMap<String, Double> assetPrices,
            BlockchainToken[] fullTokenList,
            HashMap<String, Double> nativeAssetsBalances,
            HashMap<String, HashMap<String, Double>> blockchainTokensBalances) {
        ViewPager viewPager = view.findViewById(R.id.navigation_view_pager);
        ApproveTxFragmentPageAdapter adapter =
                new ApproveTxFragmentPageAdapter(
                        getChildFragmentManager(),
                        mTxInfo,
                        txNetwork,
                        accounts,
                        assetPrices,
                        fullTokenList,
                        nativeAssetsBalances,
                        blockchainTokensBalances,
                        getActivity(),
                        mTransactionConfirmationListener == null,
                        mSolanaEstimatedTxFee);
        viewPager.setAdapter(adapter);
        viewPager.setOffscreenPageLimit(adapter.getCount() - 1);
        TabLayout tabLayout = view.findViewById(R.id.tabs);
        tabLayout.setupWithViewPager(viewPager);
    }

    private void rejectTransaction(boolean dismiss) {
        TxService txService = getTxService();
        if (txService == null) {
            return;
        }
        txService.rejectTransaction(
                mCoinType,
                mTxInfo.chainId,
                mTxInfo.id,
                success -> {
                    assert success : "tx is not rejected";
                    if (!success || !dismiss) {
                        return;
                    }
                    mRejected = true;
                    if (mTransactionConfirmationListener != null) {
                        mTransactionConfirmationListener.onRejectTransaction();
                    }
                    dismiss();
                });
    }

    private void approveTransaction() {
        TxService txService = getTxService();
        if (txService == null) {
            return;
        }
        txService.approveTransaction(
                mCoinType,
                mTxInfo.chainId,
                mTxInfo.id,
                (success, error, errorMessage) -> {
                    if (!success) {
                        int providerError = -1;
                        switch (error.which()) {
                            case ProviderErrorUnion.Tag.ProviderError:
                                providerError = error.getProviderError();
                                break;
                            case ProviderErrorUnion.Tag.SolanaProviderError:
                                providerError = error.getSolanaProviderError();
                                break;
                            case ProviderErrorUnion.Tag.FilecoinProviderError:
                                providerError = error.getFilecoinProviderError();
                                break;
                            case ProviderErrorUnion.Tag.BitcoinProviderError:
                                providerError = error.getBitcoinProviderError();
                                break;
                            default:
                                assert false : "unknown error " + errorMessage;
                        }
                        assert success
                                : "tx is not approved error: "
                                        + providerError
                                        + ", "
                                        + errorMessage;
                        Utils.warnWhenError(
                                ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT,
                                "approveTransaction",
                                providerError,
                                errorMessage);
                        return;
                    }
                    mApproved = true;
                    if (mTransactionConfirmationListener != null) {
                        mTransactionConfirmationListener.onApproveTransaction();
                    }
                    dismiss();
                });
    }

    private boolean canUpdateUi() {
        return isAdded() && getActivity() != null;
    }

    @Override
    public void onCancel(@NonNull DialogInterface dialog) {
        super.onCancel(dialog);
        if (mTransactionConfirmationListener != null) {
            mTransactionConfirmationListener.onCancel();
        }
    }
}
