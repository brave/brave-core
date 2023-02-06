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
import androidx.cardview.widget.CardView;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.chromium.base.task.PostTask;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.NftDetailActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.PendingTxHelper;
import org.chromium.chrome.browser.crypto_wallet.util.PortfolioHelper;
import org.chromium.chrome.browser.crypto_wallet.util.SmoothLineChartEquallySpaced;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.content_public.browser.UiThreadTaskTraits;

import java.util.ArrayList;
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
    private RecyclerView mRvCoins;
    private WalletCoinAdapter mWalletCoinAdapter;
    private NetworkInfo mNetworkInfo;

    private CardView mNftContainer;
    private RecyclerView mRvNft;
    private WalletCoinAdapter mWalletNftAdapter;
    private TextView mTvNftTitle;
    private SmoothLineChartEquallySpaced mChartES;
    private PortfolioModel mPortfolioModel;
    private ProgressBar mPbAssetDiscovery;
    private List<PortfolioModel.NftDataModel> mNftDataModels;

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

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Api33AndPlusBackPressHelper.create(
                    this, (FragmentActivity) requireActivity(), () -> requireActivity().finish());
        }
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            mWalletModel = activity.getWalletModel();
            mPortfolioModel = mWalletModel.getCryptoModel().getPortfolioModel();
            mWalletModel.getCryptoModel().getPortfolioModel().discoverAssetsOnAllSupportedChains();
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
                networkInfo =
                        mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.getValue();
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
        mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.observe(
                getViewLifecycleOwner(), networkInfo -> {
                    if (networkInfo == null) return;
                    if (mNetworkInfo != null && !mNetworkInfo.chainId.equals(networkInfo.chainId)) {
                        // clean up list to avoid user clicking on an asset of the previously
                        // selected network after the network has been changed
                        clearAssets();
                    }
                    mNetworkInfo = networkInfo;
                    mBtnChangeNetwork.setText(Utils.getShortNameOfNetwork(networkInfo.chainName));
                    updatePortfolioGetPendingTx();
                });
        mPortfolioModel.mNftModels.observe(getViewLifecycleOwner(), nftDataModels -> {
            if (nftDataModels.isEmpty() || mPortfolioModel.mPortfolioHelper == null) return;
            mNftDataModels = nftDataModels;
            setUpNftList(nftDataModels, mPortfolioModel.mPortfolioHelper.getPerTokenCryptoSum(),
                    mPortfolioModel.mPortfolioHelper.getPerTokenFiatSum());
        });
        // Show pending transactions fab to process pending txs
        mWalletModel.getCryptoModel().getPendingTransactions().observe(
                getViewLifecycleOwner(), transactionInfos -> {
                    mPendingTxs = transactionInfos;
                    if (mCurrentPendingTx == null && mPendingTxs.size() > 0) {
                        mCurrentPendingTx = mPendingTxs.get(0);
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
        editVisibleAssets.setOnClickListener(v -> { onEditVisibleAssetsClick(); });

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
        initNftUi(view);
    }

    private void initNftUi(View root) {
        TextView editVisibleNft = root.findViewById(R.id.edit_visible_nfts);
        mRvNft = root.findViewById(R.id.rv_nft);
        mRvNft.addItemDecoration(
                new DividerItemDecoration(requireContext(), DividerItemDecoration.VERTICAL));
        mNftContainer = root.findViewById(R.id.nft_container);
        mTvNftTitle = root.findViewById(R.id.tv_nft_title);
        editVisibleNft.setOnClickListener(v -> { onEditVisibleAssetsClick(); });
    }

    private void setUpCoinList(BlockchainToken[] userAssets,
            HashMap<String, Double> perTokenCryptoSum, HashMap<String, Double> perTokenFiatSum) {
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();

        mWalletCoinAdapter = Utils.setupVisibleAssetList(
                userAssets, perTokenCryptoSum, perTokenFiatSum, tokensPath);
        mWalletCoinAdapter.setOnWalletListItemClick(PortfolioFragment.this);
        mRvCoins.setAdapter(mWalletCoinAdapter);
        mRvCoins.setLayoutManager(new LinearLayoutManager(getActivity()));
    }

    private void setUpNftList(List<PortfolioModel.NftDataModel> nftDataModels,
            HashMap<String, Double> perTokenCryptoSum, HashMap<String, Double> perTokenFiatSum) {
        if (nftDataModels.size() == 0) {
            AndroidUtils.gone(mNftContainer, mTvNftTitle);
        } else {
            AndroidUtils.show(mNftContainer, mTvNftTitle);
        }
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();

        mWalletNftAdapter = Utils.setupVisibleNftAssetList(
                nftDataModels, perTokenCryptoSum, perTokenFiatSum, tokensPath);
        mWalletNftAdapter.setOnWalletListItemClick(PortfolioFragment.this);
        mRvNft.setAdapter(mWalletNftAdapter);
        mRvNft.setLayoutManager(new LinearLayoutManager(getActivity()));
    }

    private void clearAssets() {
        if (mWalletCoinAdapter != null) {
            mWalletCoinAdapter.clear();
        }
        if (mWalletNftAdapter != null) {
            mWalletNftAdapter.clear();
        }
        AndroidUtils.gone(mTvNftTitle, mNftContainer);
    }

    @Override
    public void onAssetClick(BlockchainToken asset) {
        NetworkInfo selectedNetwork = null;
        if (mWalletModel != null) {
            selectedNetwork =
                    mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.getValue();
        }
        if (selectedNetwork == null) {
            return;
        }

        if (asset.isErc721 || asset.isNft) {
            PortfolioModel.NftDataModel selectedNft = JavaUtils.find(mNftDataModels,
                    nftDataModel -> AssetUtils.Filters.isSameNFT(asset, nftDataModel.token));
            if (selectedNft == null) {
                return;
            }

            Intent intent = NftDetailActivity.getIntent(
                    getContext(), selectedNetwork.chainId, asset, selectedNft);
            startActivity(intent);
        } else {
            Utils.openAssetDetailsActivity(getActivity(), selectedNetwork.chainId, asset);
        }
    }

    private void openNetworkSelection() {
        BraveActivity activity = BraveActivity.getBraveActivity();
        assert activity != null;
        activity.openNetworkSelection();
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
                mChartES.setColors(new int[] {0xFFF73A1C, 0xFFBF14A2, 0xFF6F4CD2});
                mChartES.setData(mPortfolioHelper.getFiatHistory());

                AdjustTrendControls();
            });
        });
    }

    private void updatePortfolioGetPendingTx() {
        if (mWalletModel == null) return;

        final NetworkInfo selectedNetwork =
                mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.getValue();
        if (selectedNetwork == null) {
            return;
        }
        mWalletModel.getKeyringModel().getKeyringPerId(
                AssetUtils.getKeyringForCoinType(selectedNetwork.coin), keyringInfo -> {
                    if (keyringInfo == null) return;
                    AccountInfo[] accountInfos = new AccountInfo[] {};
                    if (keyringInfo != null) {
                        accountInfos = keyringInfo.accountInfos;
                    }
                    Activity activity = getActivity();
                    final AccountInfo[] accountInfosFinal = accountInfos;
                    if (!(activity instanceof BraveWalletActivity)) return;
                    LiveDataUtil.observeOnce(
                            mWalletModel.getCryptoModel().getNetworkModel().mCryptoNetworks,
                            allNetworks -> {
                                mPortfolioHelper =
                                        new PortfolioHelper((BraveWalletActivity) activity,
                                                allNetworks, accountInfosFinal);

                                mPortfolioHelper.setSelectedNetwork(selectedNetwork);
                                mPortfolioHelper.calculateBalances(() -> {
                                    final String fiatSumString = String.format(Locale.getDefault(),
                                            "$%,.2f", mPortfolioHelper.getTotalFiatSum());
                                    PostTask.runOrPostTask(UiThreadTaskTraits.DEFAULT, () -> {
                                        mFiatSumString = fiatSumString;
                                        mBalance.setText(mFiatSumString);
                                        mBalance.invalidate();

                                        List<BlockchainToken> tokens = new ArrayList<>();
                                        List<BlockchainToken> nfts = new ArrayList<>();

                                        for (BlockchainToken token :
                                                mPortfolioHelper.getUserAssets()) {
                                            if (token.isErc721 || token.isNft) {
                                                nfts.add(token);
                                            } else {
                                                tokens.add(token);
                                            }
                                        }

                                        setUpCoinList(tokens.toArray(new BlockchainToken[0]),
                                                mPortfolioHelper.getPerTokenCryptoSum(),
                                                mPortfolioHelper.getPerTokenFiatSum());

                                        mPortfolioModel.prepareNftListMetaData(
                                                nfts, mNetworkInfo, mPortfolioHelper);
                                        updatePortfolioGraph();
                                    });
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
                getFragmentManager(), ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
    }

    private void onEditVisibleAssetsClick() {
        JsonRpcService jsonRpcService = getJsonRpcService();
        assert jsonRpcService != null;
        NetworkInfo selectedNetwork = null;
        if (mWalletModel != null) {
            selectedNetwork =
                    mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.getValue();
        }
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
