/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.chromium.base.task.PostTask;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.crypto_wallet.util.PortfolioHelper;
import org.chromium.chrome.browser.crypto_wallet.util.SmoothLineChartEquallySpaced;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.content_public.browser.UiThreadTaskTraits;

import java.util.HashMap;
import java.util.List;
import java.util.Locale;

public class PortfolioFragment
        extends Fragment implements OnWalletListItemClick, ApprovedTxObserver {
    private static String TAG = "PortfolioFragment";
    private TextView mBalance;
    private Button mBtnChangeNetwork;
    private HashMap<String, TransactionInfo[]> mPendingTxInfos;

    private String mFiatSumString;

    private int mPreviousCheckedRadioId;
    private int mCurrentTimeframeType;
    private WalletModel mWalletModel;
    private List<TransactionInfo> mPendingTxs;
    private TransactionInfo mCurrentPendingTx;

    PortfolioHelper mPortfolioHelper;

    public static PortfolioFragment newInstance() {
        return new PortfolioFragment();
    }

    private JsonRpcService getJsonRpcService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getJsonRpcService();
        }

        return null;
    }

    private TxService getTxService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getTxService();
        }

        return null;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            mWalletModel = activity.getWalletModel();
        }
        View view = inflater.inflate(R.layout.fragment_portfolio, container, false);

        view.setOnTouchListener(new View.OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View v, MotionEvent event) {
                SmoothLineChartEquallySpaced chartES = view.findViewById(R.id.line_chart);
                if (chartES == null) {
                    return true;
                }
                if (event.getAction() == MotionEvent.ACTION_MOVE
                        || event.getAction() == MotionEvent.ACTION_DOWN) {
                    chartES.drawLine(event.getRawX(), mBalance);
                } else if (event.getAction() == MotionEvent.ACTION_UP
                        || event.getAction() == MotionEvent.ACTION_CANCEL) {
                    mBalance.setText(mFiatSumString);
                    mBalance.invalidate();
                    chartES.drawLine(-1, null);
                }

                return true;
            }
        });

        mBtnChangeNetwork = view.findViewById(R.id.fragment_portfolio_btn_change_networks);
        mBtnChangeNetwork.setOnClickListener(v -> { openNetworkSelection(); });
        mBtnChangeNetwork.setOnLongClickListener(v -> {
            NetworkInfo networkInfo =
                    mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.getValue();
            if (networkInfo != null) {
                Toast.makeText(requireContext(), networkInfo.chainName, Toast.LENGTH_SHORT).show();
            }
            return true;
        });

        mBalance = view.findViewById(R.id.balance);
        mBalance.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                updatePortfolioGetPendingTx();
            }
        });
        setUpObservers();
        return view;
    }

    private void setUpObservers() {
        mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.observe(
                getViewLifecycleOwner(), networkInfo -> {
                    if (networkInfo == null) return;
                    mBtnChangeNetwork.setText(Utils.getShortNameOfNetwork(networkInfo.chainName));
                    updatePortfolioGetPendingTx();
                });
        mWalletModel.getCryptoModel().getPendingTransactions().observe(
                getViewLifecycleOwner(), transactionInfos -> {
                    mPendingTxs = transactionInfos;
                    if (mCurrentPendingTx == null && mPendingTxs.size() > 0) {
                        mCurrentPendingTx = mPendingTxs.get(0);
                    }
                    updatePendingTxNotification();
                });

        mWalletModel.getCryptoModel().getNetworkModel().mNeedToCreateAccountForNetwork.observe(
                getViewLifecycleOwner(), networkInfo -> {
                    if (networkInfo == null) return;
                    MaterialAlertDialogBuilder builder =
                            new MaterialAlertDialogBuilder(
                                    requireContext(), R.style.BraveWalletAlertDialogTheme)
                                    .setMessage(getString(
                                            R.string.brave_wallet_create_account_description,
                                            networkInfo.symbolName))
                                    .setPositiveButton(R.string.wallet_action_yes,
                                            (dialog, which) -> {
                                                mWalletModel.getCryptoModel()
                                                        .getNetworkModel()
                                                        .setNetwork(networkInfo, success -> {
                                                            if (success) {
                                                                mWalletModel.getKeyringModel().addAccount(
                                                                        WalletUtils.getUniqueNextAccountName(
                                                                                requireContext(),
                                                                                mWalletModel
                                                                                        .getKeyringModel()
                                                                                        .mAccountInfos
                                                                                        .getValue()
                                                                                        .toArray(
                                                                                                new AccountInfo
                                                                                                        [0]),
                                                                                networkInfo
                                                                                        .symbolName,
                                                                                networkInfo.coin),
                                                                        networkInfo.coin,
                                                                        isAccountAdded -> {});
                                                            }
                                                            mWalletModel.getCryptoModel()
                                                                    .getNetworkModel()
                                                                    .clearCreateAccountState();
                                                        });
                                            })
                                    .setNegativeButton(
                                            R.string.wallet_action_no, (dialog, which) -> {
                                                mWalletModel.getCryptoModel()
                                                        .getNetworkModel()
                                                        .clearCreateAccountState();
                                                dialog.dismiss();
                                            });
                    builder.show();
                });
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        assert getActivity() != null;

        TextView editVisibleAssets = view.findViewById(R.id.edit_visible_assets);
        editVisibleAssets.setOnClickListener(v -> {
            JsonRpcService jsonRpcService = getJsonRpcService();
            assert jsonRpcService != null;
            NetworkInfo selectedNetwork =
                    mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.getValue();
            if (selectedNetwork == null) {
                return;
            }
            EditVisibleAssetsBottomSheetDialogFragment bottomSheetDialogFragment =
                    EditVisibleAssetsBottomSheetDialogFragment.newInstance(
                            WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST);

            bottomSheetDialogFragment.setSelectedNetwork(selectedNetwork);
            bottomSheetDialogFragment.setDismissListener(
                    new EditVisibleAssetsBottomSheetDialogFragment.DismissListener() {
                        @Override
                        public void onDismiss(Boolean isAssetsListChanged) {
                            if (isAssetsListChanged != null && isAssetsListChanged) {
                                updatePortfolioGetPendingTx();
                            }
                        }
                    });

            bottomSheetDialogFragment.show(
                    getFragmentManager(), EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
        });

        RadioGroup radioGroup = view.findViewById(R.id.portfolio_duration_radio_group);
        mPreviousCheckedRadioId = radioGroup.getCheckedRadioButtonId();
        mCurrentTimeframeType = Utils.getTimeframeFromRadioButtonId(mPreviousCheckedRadioId);
        radioGroup.setOnCheckedChangeListener((group, checkedId) -> {
            ((RadioButton) view.findViewById(mPreviousCheckedRadioId))
                    .setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
            RadioButton button = view.findViewById(checkedId);
            mCurrentTimeframeType = Utils.getTimeframeFromRadioButtonId(checkedId);
            mPreviousCheckedRadioId = checkedId;
            updatePortfolioGraph();
        });
    }

    private void setUpCoinList(BlockchainToken[] userAssets,
            HashMap<String, Double> perTokenCryptoSum, HashMap<String, Double> perTokenFiatSum) {
        View view = getView();
        assert view != null;

        RecyclerView rvCoins = view.findViewById(R.id.rvCoins);

        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();

        WalletCoinAdapter walletCoinAdapter = Utils.setupVisibleAssetList(
                userAssets, perTokenCryptoSum, perTokenFiatSum, tokensPath);
        walletCoinAdapter.setOnWalletListItemClick(PortfolioFragment.this);
        rvCoins.setAdapter(walletCoinAdapter);
        rvCoins.setLayoutManager(new LinearLayoutManager(getActivity()));
    }

    @Override
    public void onAssetClick(BlockchainToken asset) {
        NetworkInfo selectedNetwork =
                mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.getValue();
        if (selectedNetwork == null) {
            return;
        }
        Utils.openAssetDetailsActivity(getActivity(), selectedNetwork.chainId, asset);
    }

    private void openNetworkSelection() {
        BraveActivity activity = BraveActivity.getBraveActivity();
        assert activity != null;
        activity.openNetworkSelection();
    }

    private AssetRatioService getAssetRatioService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getAssetRatioService();
        }

        return null;
    }

    private KeyringService getKeyringService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getKeyringService();
        }

        return null;
    }

    BraveWalletService getBraveWalletService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getBraveWalletService();
        } else {
            assert false;
        }

        return null;
    }

    private void AdjustTrendControls() {
        TextView trendTimeframe = getView().findViewById(R.id.trend_timeframe);
        TextView trendPercentage = getView().findViewById(R.id.trend_percentage);

        if (mPortfolioHelper.isFiatHistoryEmpty()) {
            trendTimeframe.setVisibility(View.GONE);
            trendPercentage.setVisibility(View.GONE);
        } else {
            trendTimeframe.setText(Utils.getTimeframeString(mCurrentTimeframeType));

            Double currentFiatSum = mPortfolioHelper.getTotalFiatSum();
            Double mostRecentFiatSum = mPortfolioHelper.getMostRecentFiatSum();

            Double percents = ((currentFiatSum - mostRecentFiatSum) / mostRecentFiatSum) * 100;
            trendPercentage.setText(
                    String.format(Locale.getDefault(), "%.2f%%", Math.abs(percents)));
            if (mostRecentFiatSum > currentFiatSum) {
                trendPercentage.setTextColor(
                        getResources().getColor(R.color.wallet_negative_trend_color));
                trendPercentage.setCompoundDrawablesWithIntrinsicBounds(
                        R.drawable.ic_down_icon, 0, 0, 0);
            } else {
                trendPercentage.setTextColor(
                        getResources().getColor(R.color.wallet_positive_trend_color));
                trendPercentage.setCompoundDrawablesWithIntrinsicBounds(
                        R.drawable.ic_up_icon, 0, 0, 0);
            }

            trendTimeframe.setVisibility(View.VISIBLE);
            trendPercentage.setVisibility(View.VISIBLE);
        }
    }

    private void updatePortfolioGraph() {
        AssetPriceTimeframe.validate(mCurrentTimeframeType);

        if (mPortfolioHelper == null) {
            updatePortfolioGetPendingTx();
            return;
        }

        mPortfolioHelper.setFiatHistoryTimeframe(mCurrentTimeframeType);
        mPortfolioHelper.calculateFiatHistory(() -> {
            PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
                SmoothLineChartEquallySpaced chartES = getView().findViewById(R.id.line_chart);
                chartES.setColors(new int[] {0xFFF73A1C, 0xFFBF14A2, 0xFF6F4CD2});
                chartES.setData(mPortfolioHelper.getFiatHistory());

                AdjustTrendControls();
            });
        });
    }

    private void updatePortfolioGetPendingTx() {
        KeyringService keyringService = getKeyringService();
        assert keyringService != null;

        final NetworkInfo selectedNetwork =
                mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.getValue();
        if (selectedNetwork == null) {
            return;
        }
        keyringService.getKeyringInfo(
                Utils.getKeyringForCoinType(selectedNetwork.coin), keyringInfo -> {
                    AccountInfo[] accountInfos = new AccountInfo[] {};
                    if (keyringInfo != null) {
                        accountInfos = keyringInfo.accountInfos;
                    }
                    Activity activity = getActivity();
                    if (!(activity instanceof BraveWalletActivity)) return;
                    mPortfolioHelper =
                            new PortfolioHelper((BraveWalletActivity) activity, accountInfos);

                    mPortfolioHelper.setSelectedNetwork(selectedNetwork);
                    mPortfolioHelper.calculateBalances(() -> {
                        final String fiatSumString = String.format(
                                Locale.getDefault(), "$%,.2f", mPortfolioHelper.getTotalFiatSum());
                        PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
                            mFiatSumString = fiatSumString;
                            mBalance.setText(mFiatSumString);
                            mBalance.invalidate();

                            setUpCoinList(mPortfolioHelper.getUserAssets(),
                                    mPortfolioHelper.getPerTokenCryptoSum(),
                                    mPortfolioHelper.getPerTokenFiatSum());

                            updatePortfolioGraph();
                        });
                    });
                });
    }

    @Override
    public void onTxPending(String accountName, String txId) {
        updatePortfolioGetPendingTx();
    }

    @Override
    public void onTxApprovedRejected(boolean approved, String accountName, String txId) {
        updatePendingTxNotification();
        updateNextPendingTx();
        callAnotherApproveDialog();
    }

    public void callAnotherApproveDialog() {
        if (!hasPendingTx()) {
            return;
        }
        ApproveTxBottomSheetDialogFragment approveTxBottomSheetDialogFragment =
                ApproveTxBottomSheetDialogFragment.newInstance(mCurrentPendingTx,
                        mWalletModel.getCryptoModel()
                                .getPendingTxHelper()
                                .getAccountNameForTransaction(mCurrentPendingTx));
        approveTxBottomSheetDialogFragment.setApprovedTxObserver(this);
        approveTxBottomSheetDialogFragment.show(
                getFragmentManager(), ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
    }

    private void updateNextPendingTx() {
        if (mCurrentPendingTx != null) {
            for (TransactionInfo info : mPendingTxs) {
                if (!mCurrentPendingTx.id.equals(info.id)
                        && info.txStatus == TransactionStatus.UNAPPROVED) {
                    mCurrentPendingTx = info;
                    return;
                }
            }
            mCurrentPendingTx = null;
        } else if (mPendingTxs.size() > 0) {
            mCurrentPendingTx = mPendingTxs.get(0);
        }
    }

    private boolean hasPendingTx() {
        return mCurrentPendingTx != null;
    }

    private void updatePendingTxNotification() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity)
            if (hasPendingTx())
                ((BraveWalletActivity) activity).setPendingTxNotificationVisibility(View.VISIBLE);
            else
                ((BraveWalletActivity) activity).setPendingTxNotificationVisibility(View.GONE);
    }
}
