/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.NftDetailActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletNftAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.PortfolioHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.custom_layout.AutoFitVerticalGridLayoutManager;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.content_public.browser.UiThreadTaskTraits;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class NftGridFragment extends Fragment implements OnWalletListItemClick {
    private static final String TAG = "NftGridFragment";
    private static final float NFT_ITEM_WIDTH_DP = 180;

    private WalletModel mWalletModel;
    private PortfolioModel mPortfolioModel;
    private List<PortfolioModel.NftDataModel> mNftDataModels;
    private List<NetworkInfo> mAllNetworkInfos;

    private PortfolioHelper mPortfolioHelper;
    private NetworkInfo mNetworkInfo;

    private RecyclerView mRvNft;
    private WalletNftAdapter mWalletNftAdapter;
    private ProgressBar mPbAssetDiscovery;
    private ViewGroup mAddNftsContainer;

    public static NftGridFragment newInstance() {
        return new NftGridFragment();
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
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "onCreate " + e);
        }
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_nft_grid, container, false);
        mPbAssetDiscovery = view.findViewById(R.id.frag_port_pb_asset_discovery);
        mAddNftsContainer = view.findViewById(R.id.add_nfts_container);
        setUpObservers();
        return view;
    }

    // TODO: refactor this entire method once multi-chain support will be merged.
    private void setUpObservers() {
        if (mWalletModel == null) return;
        mWalletModel.getCryptoModel().getNetworkModel().mCryptoNetworks.observe(
                getViewLifecycleOwner(),
                allNetworkInfos -> { mAllNetworkInfos = allNetworkInfos; });
        mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.observe(
                getViewLifecycleOwner(), networkInfo -> {
                    if (networkInfo == null) return;
                    if (mNetworkInfo != null && !mNetworkInfo.chainId.equals(networkInfo.chainId)) {
                        // Clean up list to avoid user clicking on an asset of the previously
                        // selected network after the network has been changed.
                        clearAssets();
                    }
                    mNetworkInfo = networkInfo;
                    updateNftGrid();
                });
        mPortfolioModel.mNftModels.observe(getViewLifecycleOwner(), nftDataModels -> {
            if (mPortfolioModel.mPortfolioHelper == null) return;
            mNftDataModels = nftDataModels;
            setUpNftList(nftDataModels, mPortfolioModel.mPortfolioHelper.getPerTokenCryptoSum(),
                    mPortfolioModel.mPortfolioHelper.getPerTokenFiatSum());
        });

        mPortfolioModel.mIsDiscoveringUserAssets.observe(
                getViewLifecycleOwner(), isDiscoveringUserAssets -> {
                    if (isDiscoveringUserAssets) {
                        mPbAssetDiscovery.setVisibility(View.VISIBLE);
                    } else {
                        mPbAssetDiscovery.setVisibility(View.GONE);
                        updateNftGrid();
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
                                                setPositiveButtonAccountCreation(networkInfo);
                                            })
                                    .setNegativeButton(
                                            R.string.wallet_action_no, (dialog, which) -> {
                                                setNegativeButtonAccountCreation(dialog);
                                            });
                    builder.show();
                });
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        TextView editVisibleNft = view.findViewById(R.id.edit_visible_nfts);
        mRvNft = view.findViewById(R.id.rv_nft);
        editVisibleNft.setOnClickListener(v -> { onEditVisibleAssetsClick(); });
    }

    private void setPositiveButtonAccountCreation(NetworkInfo networkInfo) {
        mWalletModel.getCryptoModel().getNetworkModel().setNetwork(
                networkInfo, success -> { setRetrievedNetwork(networkInfo, success); });
    }

    private void setNegativeButtonAccountCreation(DialogInterface dialog) {
        mWalletModel.getCryptoModel().getNetworkModel().clearCreateAccountState();
        dialog.dismiss();
    }

    private void setRetrievedNetwork(NetworkInfo networkInfo, boolean success) {
        if (success) {
            mWalletModel.getKeyringModel().addAccount(
                    WalletUtils.getUniqueNextAccountName(requireContext(),
                            mWalletModel.getKeyringModel().mAccountInfos.getValue().toArray(
                                    new AccountInfo[0]),
                            networkInfo.symbolName, networkInfo.coin),
                    networkInfo.coin, isAccountAdded -> {});
        }
        mWalletModel.getCryptoModel().getNetworkModel().clearCreateAccountState();
    }

    private void setUpNftList(List<PortfolioModel.NftDataModel> nftDataModels,
            HashMap<String, Double> perTokenCryptoSum, HashMap<String, Double> perTokenFiatSum) {
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();

        List<WalletListItemModel> walletListItemModelList =
                Utils.createWalletListItemModel(nftDataModels, perTokenCryptoSum, perTokenFiatSum,
                        tokensPath, getResources(), mAllNetworkInfos);
        mWalletNftAdapter = new WalletNftAdapter();
        mWalletNftAdapter.setWalletListItemModelList(walletListItemModelList);
        mWalletNftAdapter.setOnWalletListItemClick(this);
        mRvNft.setAdapter(mWalletNftAdapter);
        mRvNft.setLayoutManager(
                new AutoFitVerticalGridLayoutManager(getActivity(), 2, NFT_ITEM_WIDTH_DP));

        // Show empty screen layout if no NFTs have been added.
        boolean emptyList = walletListItemModelList.isEmpty();
        mAddNftsContainer.setVisibility(emptyList ? View.VISIBLE : View.GONE);
    }

    private void clearAssets() {
        if (mWalletNftAdapter != null) {
            mWalletNftAdapter.clear();
        }
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

    private void updateNftGrid() {
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
                                    List<BlockchainToken> nfts = new ArrayList<>();
                                    for (BlockchainToken token : mPortfolioHelper.getUserAssets()) {
                                        if (token.isErc721 || token.isNft) {
                                            nfts.add(token);
                                        }
                                    }

                                    mPortfolioModel.prepareNftListMetaData(
                                            nfts, mNetworkInfo, mPortfolioHelper);
                                });
                            });
                });
    }

    private void onEditVisibleAssetsClick() {
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
                        WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST, true);

        bottomSheetDialogFragment.setSelectedNetwork(selectedNetwork);
        bottomSheetDialogFragment.setDismissListener(
                new EditVisibleAssetsBottomSheetDialogFragment.DismissListener() {
                    @Override
                    public void onDismiss(Boolean isAssetsListChanged) {
                        if (isAssetsListChanged != null && isAssetsListChanged) {
                            updateNftGrid();
                        }
                    }
                });

        bottomSheetDialogFragment.show(
                getFragmentManager(), EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
    }
}
