/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.text.method.LinkMovementMethod;
import android.text.style.UnderlineSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.tabs.TabLayout;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.domain.NetworkSelectorModel;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.app.helpers.Api33AndPlusBackPressHelper;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAssetActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.NetworkSelectorActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.NftDetailActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletNftAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.AssetUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.PortfolioHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.custom_layout.AutoFitVerticalGridLayoutManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.text.SpanApplier;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;

public class NftGridFragment extends Fragment implements OnWalletListItemClick {
    private enum SelectedTab { NFTS, HIDDEN }

    public static final String SHOW_NFT_DISCOVERY_DIALOG = "nft_discovery_dialog";
    private static final String TAG = "NftGridFragment";
    private static final float NFT_ITEM_WIDTH_DP = 180;

    private WalletModel mWalletModel;
    private PortfolioModel mPortfolioModel;
    private List<PortfolioModel.NftDataModel> mNftDataModels;
    private List<PortfolioModel.NftDataModel> mNftHiddenDataModels;
    private NetworkSelectorModel mNetworkSelectionModel;
    private boolean mCanRunOnceWhenResumed;

    private List<NetworkInfo> mAllNetworkInfos;
    private PortfolioHelper mPortfolioHelper;
    private List<NetworkInfo> mSelectedNetworkList;

    private boolean mActive;
    private RecyclerView mRvNft;
    private WalletNftAdapter mWalletNftAdapter;
    private RecyclerView mRvHiddenNft;
    private WalletNftAdapter mWalletHiddenNftAdapter;
    private ProgressBar mPbAssetDiscovery;
    private ViewGroup mAddNftsContainer;
    private ImageButton mBtnChangeNetwork;
    private ImageView mBtnAddNft;
    private TabLayout mTabLayout;
    private SelectedTab mSelectedTab;

    private ActivityResultLauncher<Intent> mAddAssetActivityResultLauncher;

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
        mSelectedNetworkList = Collections.emptyList();
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            mPortfolioModel = mWalletModel.getCryptoModel().getPortfolioModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate", e);
        }
        mCanRunOnceWhenResumed = true;
        // Initial state is NFTs section.
        mSelectedTab = SelectedTab.NFTS;
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_nft_grid, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        mTabLayout = view.findViewById(R.id.nft_tab_layout);
        mTabLayout.addTab(mTabLayout.newTab().setText(getString(R.string.brave_wallet_nfts)));
        mTabLayout.addTab(mTabLayout.newTab().setText(getString(R.string.brave_wallet_hidden)));
        mTabLayout.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
            @Override
            public void onTabSelected(TabLayout.Tab tab) {
                final boolean nftPosition = tab.getPosition() == 0;
                mRvNft.setVisibility(nftPosition ? View.VISIBLE : View.GONE);
                mRvHiddenNft.setVisibility(nftPosition ? View.GONE : View.VISIBLE);
                mSelectedTab = nftPosition ? SelectedTab.NFTS : SelectedTab.HIDDEN;
                showEmptyMessage(nftPosition ? mWalletNftAdapter : mWalletHiddenNftAdapter);
            }

            @Override
            public void onTabUnselected(TabLayout.Tab tab) {
                /* No-op. */
            }

            @Override
            public void onTabReselected(TabLayout.Tab tab) {
                /* No-op. */
            }
        });
        mPbAssetDiscovery = view.findViewById(R.id.frag_nft_grid_pb_asset_discovery);
        mAddNftsContainer = view.findViewById(R.id.add_nfts_container);
        mBtnChangeNetwork = view.findViewById(R.id.fragment_nft_grid_btn_change_networks);
        mBtnChangeNetwork.setOnClickListener(v -> openNetworkSelection());

        mBtnAddNft = view.findViewById(R.id.fragment_nft_grid_btn_add_nft);
        mBtnAddNft.setOnClickListener(v -> {
            Intent addAssetIntent = AddAssetActivity.getIntent(requireContext(), true);
            mAddAssetActivityResultLauncher.launch(addAssetIntent);
        });
        setUpObservers();

        // Pass @{code ActivityResultRegistry} reference explicitly to avoid crash
        // https://github.com/brave/brave-browser/issues/31882
        mAddAssetActivityResultLauncher =
                registerForActivityResult(new ActivityResultContracts.StartActivityForResult(),
                        ((BraveWalletBaseActivity) requireActivity()).getActivityResultRegistry(),
                        result -> {
                            if (result.getResultCode() == Activity.RESULT_OK) {
                                mPbAssetDiscovery.setVisibility(View.VISIBLE);
                                updateNftGrid();
                            }
                        });

        TextView editVisibleNft = view.findViewById(R.id.edit_visible_nfts);
        mRvNft = view.findViewById(R.id.rv_nft);
        mRvHiddenNft = view.findViewById(R.id.rv_hidden_nft);
        editVisibleNft.setOnClickListener(v -> showEditVisibleDialog());
    }

    private void showEmptyMessage(@Nullable final WalletNftAdapter adapter) {
        final boolean showEmptyMessage = adapter == null || adapter.getItemCount() == 0;
        mAddNftsContainer.setVisibility(showEmptyMessage ? View.VISIBLE : View.GONE);
    }

    @Override
    public void onResume() {
        super.onResume();
        mActive = true;
        recordP3AView();

        if (JavaUtils.anyNull(mWalletModel)
                || !mCanRunOnceWhenResumed
                || !ChromeSharedPreferences.getInstance()
                        .readBoolean(SHOW_NFT_DISCOVERY_DIALOG, true)) return;

        mWalletModel.getCryptoModel().isNftDiscoveryEnabled(isNftDiscoveryEnabled -> {
            if (!isNftDiscoveryEnabled) {
                showNftDiscoveryDialog();
                mCanRunOnceWhenResumed = false;
            }
        });
    }

    @Override
    public void onPause() {
        super.onPause();
        mActive = false;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        if (mPortfolioModel != null) {
            mPortfolioModel.clearNftModels();
        }
    }

    @MainThread
    private void setUpObservers() {
        ThreadUtils.assertOnUiThread();
        if (mWalletModel == null) return;
        getNetworkModel().mCryptoNetworks.observe(
                getViewLifecycleOwner(), allNetworkInfos -> mAllNetworkInfos = allNetworkInfos);
        mNetworkSelectionModel = getNetworkModel().openNetworkSelectorModel(TAG,
                NetworkSelectorModel.Mode.LOCAL_NETWORK_FILTER,
                NetworkSelectorModel.SelectionMode.MULTI, getLifecycle());
        mNetworkSelectionModel.mSelectedNetworks.observe(
                getViewLifecycleOwner(), selectedNetworkInfos -> {
                    mSelectedNetworkList = selectedNetworkInfos;
                    if (mSelectedNetworkList.isEmpty()) {
                        mAddNftsContainer.setVisibility(View.VISIBLE);
                        // Clean up list to avoid user clicking on an asset of the previously
                        // selected network after the network has been changed.
                        clearAssets();
                        return;
                    }
                    mPbAssetDiscovery.setVisibility(View.VISIBLE);
                    updateNftGrid();
                });

        mPortfolioModel.mNftModels.observe(getViewLifecycleOwner(), nftDataModels -> {
            if (mPortfolioModel.mPortfolioHelper == null) return;
            mNftDataModels = nftDataModels;
            recordP3AView();
            mWalletNftAdapter = new WalletNftAdapter();
            setUpNftList(mRvNft, mWalletNftAdapter, nftDataModels,
                    mPortfolioModel.mPortfolioHelper.getPerTokenCryptoSum(),
                    mPortfolioModel.mPortfolioHelper.getPerTokenFiatSum(), SelectedTab.NFTS);
            mPbAssetDiscovery.setVisibility(View.INVISIBLE);
        });

        mPortfolioModel.mNftHiddenModels.observe(getViewLifecycleOwner(), nftHiddenDataModels -> {
            if (mPortfolioModel.mPortfolioHelper == null) return;
            mNftHiddenDataModels = nftHiddenDataModels;
            mWalletHiddenNftAdapter = new WalletNftAdapter();
            setUpNftList(mRvHiddenNft, mWalletHiddenNftAdapter, nftHiddenDataModels,
                    mPortfolioModel.mPortfolioHelper.getPerTokenCryptoSum(),
                    mPortfolioModel.mPortfolioHelper.getPerTokenFiatSum(), SelectedTab.HIDDEN);
        });

        mPortfolioModel.mIsDiscoveringUserAssets.observe(
                getViewLifecycleOwner(), isDiscoveringUserAssets -> {
                    if (isDiscoveringUserAssets) {
                        mPbAssetDiscovery.setVisibility(View.VISIBLE);
                    } else {
                        updateNftGrid();
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
                                            (dialog, which)
                                                    -> mWalletModel
                                                               .createAccountAndSetDefaultNetwork(
                                                                       networkInfo))
                                    .setNegativeButton(
                                            R.string.brave_action_no, (dialog, which) -> {
                                                getNetworkModel().clearCreateAccountState();
                                                dialog.dismiss();
                                            });
                    builder.show();
                });
    }

    private void showNftDiscoveryDialog() {
        if (JavaUtils.anyNull(mWalletModel)) return;

        var nftDiscoveryDesc = SpanApplier.applySpans(
                getString(R.string.brave_wallet_enable_nft_autodiscovery_modal_description),
                new SpanApplier.SpanInfo("<LINK_1>", "</LINK_1>", new UnderlineSpan()),
                new SpanApplier.SpanInfo("<LINK_2>", "</LINK_2>",
                        new NoUnderlineClickableSpan(requireContext(), R.color.brave_link,
                                result
                                -> TabUtils.openUrlInCustomTab(requireContext(),
                                        WalletConstants.NFT_DISCOVERY_LEARN_MORE_LINK))));

        MaterialAlertDialogBuilder nftDiscoveryDialogBuilder = new MaterialAlertDialogBuilder(
                requireContext(), R.style.BraveWalletAlertDialogTheme);

        View dialogView = getLayoutInflater().inflate(R.layout.dialog_alert_template, null);
        TextView title = dialogView.findViewById(R.id.dialog_tv_title);
        TextView message = dialogView.findViewById(R.id.dialog_tv_msg);
        Button cancelBtn = dialogView.findViewById(R.id.dialog_wallet_btn_negative);
        Button enableBtn = dialogView.findViewById(R.id.dialog_wallet_btn_positive);

        title.setText(R.string.brave_wallet_enable_nft_autodiscovery_modal_header);
        message.setMovementMethod(LinkMovementMethod.getInstance());
        message.setText(nftDiscoveryDesc);
        enableBtn.setText(R.string.brave_action_enable);
        nftDiscoveryDialogBuilder.setView(dialogView);
        var dialog = nftDiscoveryDialogBuilder.create();

        enableBtn.setOnClickListener(
                v -> {
                    dialog.dismiss();
                    mWalletModel.getCryptoModel().updateNftDiscovery(true);
                    ChromeSharedPreferences.getInstance()
                            .writeBoolean(SHOW_NFT_DISCOVERY_DIALOG, false);
                });
        cancelBtn.setOnClickListener(
                v -> {
                    dialog.dismiss();
                    ChromeSharedPreferences.getInstance()
                            .writeBoolean(SHOW_NFT_DISCOVERY_DIALOG, false);
                });
        dialog.show();
    }

    private void openNetworkSelection() {
        Intent intent = NetworkSelectorActivity.createIntent(
                requireContext(), NetworkSelectorModel.Mode.LOCAL_NETWORK_FILTER, TAG);
        startActivity(intent);
    }

    private void setUpNftList(RecyclerView recyclerView, WalletNftAdapter adapter,
            List<PortfolioModel.NftDataModel> nftDataModels,
            HashMap<String, Double> perTokenCryptoSum, HashMap<String, Double> perTokenFiatSum,
            SelectedTab selectedTab) {
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();

        List<WalletListItemModel> walletListItemModelList =
                Utils.createWalletListItemModel(nftDataModels, perTokenCryptoSum, perTokenFiatSum,
                        tokensPath, getResources(), mAllNetworkInfos);
        adapter.setWalletListItemModelList(walletListItemModelList);
        adapter.setOnWalletListItemClick(this);
        recyclerView.setAdapter(adapter);
        recyclerView.setLayoutManager(
                new AutoFitVerticalGridLayoutManager(getActivity(), 2, NFT_ITEM_WIDTH_DP));

        if (mSelectedTab == selectedTab) {
            // Show empty screen layout if no NFTs have been added.
            boolean emptyList = walletListItemModelList.isEmpty();
            mAddNftsContainer.setVisibility(emptyList ? View.VISIBLE : View.GONE);
        }
    }

    private void clearAssets() {
        if (mWalletNftAdapter != null) {
            mWalletNftAdapter.clear();
        }

        if (mWalletHiddenNftAdapter != null) {
            mWalletHiddenNftAdapter.clear();
        }
    }

    @Override
    public void onAssetClick(BlockchainToken asset) {
        PortfolioModel.NftDataModel selectedNft = JavaUtils.find(mNftDataModels,
                nftDataModel -> AssetUtils.Filters.isSameNFT(asset, nftDataModel.token));
        if (selectedNft == null) {
            PortfolioModel.NftDataModel selectedHiddenNft = JavaUtils.find(mNftHiddenDataModels,
                    nftHiddenDataModel
                    -> AssetUtils.Filters.isSameNFT(asset, nftHiddenDataModel.token));
            if (selectedHiddenNft == null) {
                return;
            }
            selectedNft = selectedHiddenNft;
        }

        Intent intent = NftDetailActivity.getIntent(
                getContext(), selectedNft.networkInfo.chainId, asset, selectedNft);
        startActivity(intent);
    }

    private void updateNftGrid() {
        if (mWalletModel == null || mSelectedNetworkList.isEmpty()) return;

        Activity activity = getActivity();
        if (!(activity instanceof BraveWalletActivity)) return;
        mPortfolioModel.fetchNfts(this, mSelectedNetworkList, (BraveWalletBaseActivity) activity,
                (portfolioHelper) -> { mPortfolioHelper = portfolioHelper; });
    }

    private void showEditVisibleDialog() {
        EditVisibleAssetsBottomSheetDialogFragment bottomSheetDialogFragment =
                EditVisibleAssetsBottomSheetDialogFragment.newInstance(
                        WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST, true);

        bottomSheetDialogFragment.setDismissListener(isAssetsListChanged -> {
            if (isAssetsListChanged) {
                mPbAssetDiscovery.setVisibility(View.VISIBLE);
                updateNftGrid();
            }
        });

        bottomSheetDialogFragment.show(getParentFragmentManager(),
                EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
    }

    private void recordP3AView() {
        if (!mActive) {
            return;
        }
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            BraveWalletActivity walletActivity = (BraveWalletActivity) activity;
            BraveWalletP3a walletP3A = walletActivity.getBraveWalletP3A();
            if (walletP3A != null && mNftDataModels != null) {
                walletP3A.recordNftGalleryView(mNftDataModels.size());
            }
        }
    }

    private NetworkModel getNetworkModel() {
        return mWalletModel.getCryptoModel().getNetworkModel();
    }
}
