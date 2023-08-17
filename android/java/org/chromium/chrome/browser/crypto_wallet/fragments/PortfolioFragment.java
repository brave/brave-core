/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.domain.NetworkSelectorModel;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.NetworkSelectorActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.PortfolioHelper;
import org.chromium.chrome.browser.crypto_wallet.util.SmoothLineChartEquallySpaced;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.util.LiveDataUtil;

import java.util.HashMap;
import java.util.List;
import java.util.Locale;

public class PortfolioFragment
        extends Fragment implements OnWalletListItemClick, ApprovedTxObserver {
    private static final String TAG = "PortfolioFragment";
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
    private RecyclerView mRvCoins;
    private WalletCoinAdapter mWalletCoinAdapter;
    private NetworkInfo mNetworkInfo;

    private SmoothLineChartEquallySpaced mChartES;
    private PortfolioModel mPortfolioModel;
    private ProgressBar mPbAssetDiscovery;
    private List<NetworkInfo> mAllNetworkInfos;
    private NetworkSelectorModel mNetworkSelectionModel;

    public static PortfolioFragment newInstance() {
        return new PortfolioFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Api33AndPlusBackPressHelper.create(
                    this, (FragmentActivity) requireActivity(), () -> requireActivity().finish());
        }
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            mPortfolioModel = mWalletModel.getCryptoModel().getPortfolioModel();
            mWalletModel.getCryptoModel().getPortfolioModel().discoverAssetsOnAllSupportedChains();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate " + e);
        }
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_portfolio, container, false);
        mRvCoins = view.findViewById(R.id.rvCoins);
        mChartES = view.findViewById(R.id.line_chart);
        mPbAssetDiscovery = view.findViewById(R.id.frag_port_pb_asset_discovery);
        mRvCoins.addItemDecoration(
                new DividerItemDecoration(requireContext(), DividerItemDecoration.VERTICAL));

        mChartES.setOnTouchListener(new View.OnTouchListener() {
            @Override
            @SuppressLint("ClickableViewAccessibility")
            public boolean onTouch(View v, MotionEvent event) {
                v.getParent().requestDisallowInterceptTouchEvent(true);
                if (mChartES == null) {
                    return true;
                }
                if (event.getAction() == MotionEvent.ACTION_MOVE
                        || event.getAction() == MotionEvent.ACTION_DOWN) {
                    mChartES.drawLine(event.getRawX(), mBalance);
                } else if (event.getAction() == MotionEvent.ACTION_UP
                        || event.getAction() == MotionEvent.ACTION_CANCEL) {
                    mBalance.setText(mFiatSumString);
                    mBalance.invalidate();
                    mChartES.drawLine(-1, null);
                }

                return true;
            }
        });

        mBtnChangeNetwork = view.findViewById(R.id.fragment_portfolio_btn_change_networks);
        mBtnChangeNetwork.setOnClickListener(v -> { openNetworkSelection(); });
        mBtnChangeNetwork.setOnLongClickListener(v -> {
            NetworkInfo networkInfo = null;
            if (mWalletModel != null) {
                networkInfo = mNetworkInfo;
            }
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
        if (mWalletModel != null) setUpObservers();
        return view;
    }

    private void setUpObservers() {
        getNetworkModel().mCryptoNetworks.observe(getViewLifecycleOwner(),
                allNetworkInfos -> { mAllNetworkInfos = allNetworkInfos; });
        mNetworkSelectionModel = getNetworkModel().openNetworkSelectorModel(PortfolioFragment.TAG,
                NetworkSelectorModel.Mode.LOCAL_NETWORK_FILTER, getLifecycle());
        // Show pending transactions fab to process pending txs
        mNetworkSelectionModel.getSelectedNetwork().observe(
                getViewLifecycleOwner(), localNetworkInfo -> {
                    if (localNetworkInfo == null) return;
                    if (mNetworkInfo != null
                            && !mNetworkInfo.chainId.equals(localNetworkInfo.chainId)) {
                        // Clean up list to avoid user clicking on an asset of the previously
                        // selected network after the network has been changed
                        clearAssets();
                    }
                    mNetworkInfo = localNetworkInfo;
                    mBtnChangeNetwork.setText(
                            Utils.getShortNameOfNetwork(localNetworkInfo.chainName));
                    updatePortfolioGetPendingTx();
                });
        // Show pending transactions fab to process pending txs
        mWalletModel.getCryptoModel().getPendingTransactions().observe(
                getViewLifecycleOwner(), transactionInfos -> {
                    mPendingTxs = transactionInfos;
                    if (mCurrentPendingTx == null && mPendingTxs.size() > 0) {
                        mCurrentPendingTx = mPendingTxs.get(0);
                    } else if (mPendingTxs.size() == 0) {
                        mCurrentPendingTx = null;
                    }
                    updatePendingTxNotification();
                });
        mWalletModel.getCryptoModel().getPortfolioModel().mIsDiscoveringUserAssets.observe(
                getViewLifecycleOwner(), isDiscoveringUserAssets -> {
                    if (isDiscoveringUserAssets) {
                        AndroidUtils.show(mPbAssetDiscovery);
                    } else {
                        AndroidUtils.gone(mPbAssetDiscovery);
                        updatePortfolioGetPendingTx();
                    }
                });

        getNetworkModel().mNeedToCreateAccountForNetwork.observe(
                getViewLifecycleOwner(), networkInfo -> {
                    if (networkInfo == null) return;

                    MaterialAlertDialogBuilder builder =
                            new MaterialAlertDialogBuilder(
                                    requireContext(), R.style.BraveWalletAlertDialogTheme)
                                    .setMessage(getString(
                                            R.string.brave_wallet_create_account_description,
                                            networkInfo.symbolName))
                                    .setPositiveButton(R.string.brave_action_yes,
                                            (dialog, which) -> {
                                                mWalletModel.createAccountAndSetDefaultNetwork(
                                                        networkInfo);
                                            })
                                    .setNegativeButton(
                                            R.string.brave_action_no, (dialog, which) -> {
                                                getNetworkModel().clearCreateAccountState();
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
        editVisibleAssets.setOnClickListener(v -> { showEditVisibleDialog(); });

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

    private void setUpCoinList(List<BlockchainToken> userAssets,
            HashMap<String, Double> perTokenCryptoSum, HashMap<String, Double> perTokenFiatSum,
            List<NetworkInfo> networkInfos) {
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();

        mWalletCoinAdapter = Utils.setupVisibleAssetList(userAssets, perTokenCryptoSum,
                perTokenFiatSum, tokensPath, getResources(), networkInfos);
        mWalletCoinAdapter.setOnWalletListItemClick(PortfolioFragment.this);
        mRvCoins.setAdapter(mWalletCoinAdapter);
        mRvCoins.setLayoutManager(new LinearLayoutManager(getActivity()));
    }

    private void clearAssets() {
        if (mWalletCoinAdapter != null) {
            mWalletCoinAdapter.clear();
        }
    }

    @Override
    public void onAssetClick(BlockchainToken asset) {
        NetworkInfo selectedNetwork = null;
        if (mWalletModel != null) {
            selectedNetwork = getNetworkModel().getNetwork(asset.chainId);
        }
        if (selectedNetwork == null) {
            return;
        }

        Utils.openAssetDetailsActivity(getActivity(), asset);
    }

    private void openNetworkSelection() {
        Intent intent = NetworkSelectorActivity.createIntent(getContext(),
                NetworkSelectorModel.Mode.LOCAL_NETWORK_FILTER, PortfolioFragment.TAG);
        startActivity(intent);
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
            PostTask.runOrPostTask(TaskTraits.UI_DEFAULT, () -> {
                mChartES.setColors(new int[] {0xFFF73A1C, 0xFFBF14A2, 0xFF6F4CD2});
                mChartES.setData(mPortfolioHelper.getFiatHistory());

                AdjustTrendControls();
            });
        });
    }

    private void updatePortfolioGetPendingTx() {
        if (mWalletModel == null) return;

        final NetworkInfo selectedNetwork = mNetworkInfo;
        if (selectedNetwork == null) {
            return;
        }

        Activity activity = getActivity();
        if (!(activity instanceof BraveWalletActivity)) return;
        mPortfolioModel.fetchAssetsByType(TokenUtils.TokenType.NON_NFTS, selectedNetwork,
                (BraveWalletBaseActivity) activity, (portfolioHelper) -> {
                    if (!AndroidUtils.canUpdateFragmentUi(this)) return;
                    mPortfolioHelper = portfolioHelper;

                    final String fiatSumString = String.format(
                            Locale.getDefault(), "$%,.2f", mPortfolioHelper.getTotalFiatSum());

                    mFiatSumString = fiatSumString;
                    mBalance.setText(mFiatSumString);
                    mBalance.invalidate();

                    LiveDataUtil.observeOnce(
                            getNetworkModel().mCryptoNetworks, networkInfos -> {
                                setUpCoinList(mPortfolioHelper.getUserAssets(),
                                        mPortfolioHelper.getPerTokenCryptoSum(),
                                        mPortfolioHelper.getPerTokenFiatSum(), networkInfos);
                            });
                    updatePortfolioGraph();
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
        if (!hasPendingTx() || mWalletModel == null) {
            return;
        }
        ApproveTxBottomSheetDialogFragment approveTxBottomSheetDialogFragment =
                ApproveTxBottomSheetDialogFragment.newInstance(mCurrentPendingTx,
                        mWalletModel.getCryptoModel()
                                .getPendingTxHelper()
                                .getAccountNameForTransaction(mCurrentPendingTx));
        approveTxBottomSheetDialogFragment.setApprovedTxObserver(this);
        approveTxBottomSheetDialogFragment.show(
                getParentFragmentManager(), ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
    }

    private void showEditVisibleDialog() {
        if (mNetworkInfo == null) {
            return;
        }

        EditVisibleAssetsBottomSheetDialogFragment bottomSheetDialogFragment =
                EditVisibleAssetsBottomSheetDialogFragment.newInstance(
                        WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST, false);

        bottomSheetDialogFragment.setSelectedNetwork(mNetworkInfo);
        bottomSheetDialogFragment.setDismissListener(
                new EditVisibleAssetsBottomSheetDialogFragment.DismissListener() {
                    @Override
                    public void onDismiss(boolean isAssetsListChanged) {
                        if (isAssetsListChanged) {
                            updatePortfolioGetPendingTx();
                        }
                    }
                });

        bottomSheetDialogFragment.show(
                getFragmentManager(), EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
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
            ((BraveWalletActivity) activity).showPendingTxNotification(hasPendingTx());
    }

    private NetworkModel getNetworkModel() {
        return mWalletModel.getCryptoModel().getNetworkModel();
    }
}
