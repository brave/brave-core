/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.app.Dialog;
import android.content.Context;
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
import android.webkit.URLUtil;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.fragment.app.FragmentManager;
import androidx.viewpager.widget.ViewPager;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.tabs.TabLayout;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetPrice;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.ProviderErrorUnion;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.ApproveTxFragmentPageAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.TransactionListener;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.ParsedTransaction;
import org.chromium.chrome.browser.crypto_wallet.util.SolanaTransactionsGasHelper;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@NullMarked
public class ApproveTxBottomSheetDialogFragment extends WalletBottomSheetDialogFragment {
    private static final String TAG = "ApproveTxBottomSheet";

    private final ExecutorService mExecutor;
    private final Handler mHandler;

    @MonotonicNonNull private TransactionInfo mTxInfo;
    @MonotonicNonNull private TransactionListener mTransactionListener;

    private List<TransactionInfo> mPendingTransactions;
    private Button mRejectAllTx;
    @CoinType.EnumType private int mCoinType;
    private long mSolanaEstimatedTxFee;

    public ApproveTxBottomSheetDialogFragment() {
        super();
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mSolanaEstimatedTxFee = 0;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof TransactionListener) {
            mTransactionListener = (TransactionListener) context;
            mTxInfo = mTransactionListener.getCurrentTransaction();
            if (mTxInfo == null) {
                dismissNow();
            }
            mPendingTransactions = mTransactionListener.getPendingTransactions();
        } else {
            throw new IllegalStateException(
                    "Host activity must implement TransactionConfirmationListener.");
        }
    }

    public void show(final FragmentManager manager) {
        super.show(manager, TAG);
    }

    private void setupFullHeight(final BottomSheetDialog bottomSheetDialog) {
        final FrameLayout frameLayout = bottomSheetDialog.findViewById(R.id.design_bottom_sheet);
        if (frameLayout == null) {
            assert false : "Null frame layout for bottom sheet dialog resource design_bottom_sheet";
            return;
        }

        ViewGroup.LayoutParams layoutParams = frameLayout.getLayoutParams();
        layoutParams.height = ViewGroup.LayoutParams.MATCH_PARENT;
        frameLayout.setLayoutParams(layoutParams);

        final BottomSheetBehavior<FrameLayout> behavior = BottomSheetBehavior.from(frameLayout);
        behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
    }

    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        BottomSheetDialog bottomSheetDialog =
                new BottomSheetDialog(requireContext(), R.style.ApproveTxBottomSheetDialogTheme);
        bottomSheetDialog.setOnShowListener(dialog -> setupFullHeight((BottomSheetDialog) dialog));
        return bottomSheetDialog;
    }

    @Nullable
    @Override
    public View onCreateView(
            LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.approve_tx_bottom_sheet, container, false);
    }

    @Override
    public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
        if (mTxInfo == null) {
            return;
        }
        TextView networkName = view.findViewById(R.id.network_name);
        mRejectAllTx = view.findViewById(R.id.btn_reject_transactions);
        if (mTxInfo.txType
                == TransactionType
                        .SOLANA_SPL_TOKEN_TRANSFER_WITH_ASSOCIATED_TOKEN_ACCOUNT_CREATION) {
            TextView associatedSplTokenInfo =
                    view.findViewById(R.id.tv_approve_dialog_additional_details);
            associatedSplTokenInfo.setVisibility(View.VISIBLE);
            Spanned associatedSPLTokenAccountInfo =
                    Utils.createSpanForSurroundedPhrase(
                            requireContext(),
                            R.string.brave_wallet_confirm_transaction_account_creation_fee,
                            (v) -> {
                                TabUtils.openUrlInNewTab(false, Utils.BRAVE_SUPPORT_URL);
                                TabUtils.bringChromeTabbedActivityToTheTop(getActivity());
                            });
            associatedSplTokenInfo.setMovementMethod(LinkMovementMethod.getInstance());
            associatedSplTokenInfo.setText(associatedSPLTokenAccountInfo);
        }
        mCoinType = TransactionUtils.getCoinFromTxDataUnion(mTxInfo.txDataUnion);
        final NetworkInfo txNetwork =
                getWalletModel().getNetworkModel().getNetwork(mTxInfo.chainId);
        if (txNetwork == null) {
            assert false : "Null Network info for chain ID " + mTxInfo.chainId;
            return;
        }
        networkName.setText(txNetwork.chainName);
        getKeyringService()
                .getAllAccounts(
                        allAccounts -> {
                            AccountInfo[] accounts =
                                    AssetUtils.filterAccountsByNetwork(
                                            allAccounts.accounts,
                                            txNetwork.coin,
                                            txNetwork.chainId);

                            AccountInfo txAccountInfo =
                                    Utils.findAccount(accounts, mTxInfo.fromAccountId);
                            if (txAccountInfo == null) {
                                return;
                            }

                            ImageView icon = view.findViewById(R.id.account_picture);
                            Utils.setBlockiesBitmapResourceFromAccount(
                                    mExecutor, mHandler, icon, txAccountInfo, true);

                            // First fill in data that does not require remote queries
                            TokenUtils.getAllTokensFiltered(
                                    getBraveWalletService(),
                                    getWalletModel().getBlockchainRegistry(),
                                    txNetwork,
                                    TokenUtils.TokenType.ALL,
                                    tokenList -> {
                                        SolanaTransactionsGasHelper solanaTransactionsGasHelper =
                                                new SolanaTransactionsGasHelper(
                                                        (BraveWalletBaseActivity) getActivity(),
                                                        new TransactionInfo[] {mTxInfo});
                                        solanaTransactionsGasHelper.maybeGetSolanaGasEstimations(
                                                () -> {
                                                    HashMap<String, Long> perTxFee =
                                                            solanaTransactionsGasHelper
                                                                    .getPerTxFee();
                                                    if (perTxFee.get(mTxInfo.id) != null) {
                                                        Long solTxFee = perTxFee.get(mTxInfo.id);
                                                        if (solTxFee != null) {
                                                            mSolanaEstimatedTxFee = solTxFee;
                                                        }
                                                    }
                                                    if (!canUpdateUi()) return;
                                                    ParsedTransaction parsedTx =
                                                            fillAssetDependentControls(
                                                                    view,
                                                                    txNetwork,
                                                                    txAccountInfo,
                                                                    accounts,
                                                                    new ArrayList<>(),
                                                                    tokenList,
                                                                    mSolanaEstimatedTxFee);

                                                    // Get tokens involved in this transaction
                                                    List<BlockchainToken> tokens =
                                                            new ArrayList<>();
                                                    tokens.add(Utils.makeNetworkAsset(txNetwork));
                                                    // Always add native assets.
                                                    if (parsedTx.getIsSwap()) {
                                                        tokens.add(parsedTx.getSellToken());
                                                        tokens.add(parsedTx.getBuyToken());
                                                    } else if (parsedTx.getToken() != null) {
                                                        tokens.add(parsedTx.getToken());
                                                    }
                                                    BlockchainToken[] filterByTokens =
                                                            tokens.toArray(new BlockchainToken[0]);

                                                    fetchTxBalanceAndUpdateUi(
                                                            view,
                                                            txNetwork,
                                                            txAccountInfo,
                                                            accounts,
                                                            filterByTokens);
                                                });
                                    });
                        });
        Button reject = view.findViewById(R.id.reject);
        reject.setOnClickListener(v -> rejectTransaction());
        Button approve = view.findViewById(R.id.approve);
        approve.setOnClickListener(v -> approveTransaction());
        if (mPendingTransactions.size() > 1) {
            // TODO: next button is not functional. Update next button text based on position in
            //  mTransactionInfos list.
            //  Refactor Approve pendingTxHelper code to update the mSelectedPendingRequest Tx
            //  with the next Transaction.
            Button next = view.findViewById(R.id.btn_next_tx);
            next.setVisibility(View.VISIBLE);
            next.setOnClickListener(v -> mTransactionListener.onNextTransaction());
            mRejectAllTx.setVisibility(View.VISIBLE);
            mRejectAllTx.setOnClickListener(
                    v -> {
                        mTransactionListener.onRejectAllTransactions();
                        dismiss();
                    });
            refreshListContentUi();
        }

        if (mTxInfo.originInfo != null && URLUtil.isValidUrl(mTxInfo.originInfo.originSpec)) {
            TextView domain = view.findViewById(R.id.domain);
            domain.setVisibility(View.VISIBLE);
            domain.setText(Utils.geteTldSpanned(mTxInfo.originInfo));
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        dismiss();
    }

    public void setTxList(List<TransactionInfo> transactionInfos) {
        mPendingTransactions = transactionInfos;
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
        LiveDataUtil.observeOnce(
                getWalletModel().getCryptoModel().getNetworkModel().mCryptoNetworks,
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
                                        mSolanaEstimatedTxFee);
                            });
                });
    }

    private void refreshListContentUi() {
        if (mPendingTransactions == null) {
            return;
        }
        mRejectAllTx.setText(
                getString(
                        R.string.brave_wallet_queue_reject_all,
                        String.valueOf(mPendingTransactions.size())));
    }

    private ParsedTransaction fillAssetDependentControls(
            final View view,
            NetworkInfo txNetwork,
            AccountInfo txAccountInfo,
            AccountInfo[] accounts,
            List<AssetPrice> assetPrices,
            BlockchainToken[] fullTokenList,
            long solanaEstimatedTxFee) {
        ParsedTransaction parsedTx =
                ParsedTransaction.parseTransaction(
                        mTxInfo,
                        txNetwork,
                        accounts,
                        assetPrices,
                        solanaEstimatedTxFee,
                        fullTokenList);
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
            if (parsedTx.isSolChangeOfOwnership()) {
                view.findViewById(R.id.warning_container).setVisibility(View.VISIBLE);
                view.findViewById(R.id.tab_top_space).setVisibility(View.GONE);
            }
        } else if (parsedTx.isShielded()) {
            txType.setText(getResources().getString(R.string.brave_wallet_shielding));
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
        setupPager(view, txNetwork, accounts, assetPrices, fullTokenList);
        return parsedTx;
    }

    private void setupPager(
            final View view,
            NetworkInfo txNetwork,
            AccountInfo[] accounts,
            List<AssetPrice> assetPrices,
            BlockchainToken[] fullTokenList) {
        ViewPager viewPager = view.findViewById(R.id.navigation_view_pager);
        ApproveTxFragmentPageAdapter adapter =
                new ApproveTxFragmentPageAdapter(
                        getChildFragmentManager(),
                        mTxInfo,
                        txNetwork,
                        accounts,
                        assetPrices,
                        fullTokenList,
                        requireActivity(),
                        false,
                        mSolanaEstimatedTxFee);
        viewPager.setAdapter(adapter);
        viewPager.setOffscreenPageLimit(adapter.getCount() - 1);
        TabLayout tabLayout = view.findViewById(R.id.tabs);
        tabLayout.setupWithViewPager(viewPager);
    }

    private void rejectTransaction() {
        TxService txService = getWalletModel().getTxService();
        if (txService == null) {
            return;
        }
        txService.rejectTransaction(
                mCoinType,
                mTxInfo.chainId,
                mTxInfo.id,
                success -> {
                    assert success : "tx is not rejected";
                    dismiss();
                });
    }

    private void approveTransaction() {
        TxService txService = getWalletModel().getTxService();
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
                        assert false
                                : "tx is not approved error: "
                                        + providerError
                                        + ", "
                                        + errorMessage;
                        Utils.warnWhenError(TAG, "approveTransaction", providerError, errorMessage);
                        return;
                    }
                    dismiss();
                });
    }

    /**
     * @noinspection BooleanMethodIsAlwaysInverted
     */
    private boolean canUpdateUi() {
        return isAdded() && getActivity() != null;
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        super.onDismiss(dialog);
        mTransactionListener.onCancel();
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.showWalletPanel(false, true);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onHide", e);
        }
    }
}
