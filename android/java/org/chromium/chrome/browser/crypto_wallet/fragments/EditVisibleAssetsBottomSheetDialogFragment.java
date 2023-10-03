/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import static java.lang.Boolean.compare;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.widget.SearchView;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.UserAssetModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAssetActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSpinnerAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserverImpl;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserverImpl.KeyringServiceObserverImplDelegate;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.NetworkUtils;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.util.LiveDataUtil;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.stream.IntStream;

public class EditVisibleAssetsBottomSheetDialogFragment extends BottomSheetDialogFragment
        implements View.OnClickListener, OnWalletListItemClick, KeyringServiceObserverImplDelegate,
                   AdapterView.OnItemSelectedListener {
    public static final String TAG_FRAGMENT =
            EditVisibleAssetsBottomSheetDialogFragment.class.getName();
    private WalletCoinAdapter mWalletCoinAdapter;
    private final WalletCoinAdapter.AdapterType mType;
    private final boolean mNftsOnly;
    private NetworkInfo mSelectedNetwork;
    private DismissListener mDismissListener;
    private boolean mIsAssetsListChanged;
    private static final String TAG = "EditVisibleAssetsFrag";
    private WalletModel mWalletModel;
    private KeyringServiceObserverImpl mKeyringServiceObserver;
    private OnEditVisibleItemClickListener mOnEditVisibleItemClickListener;
    private List<NetworkInfo> mCryptoNetworks;
    private UserAssetModel mUserAssetModel;
    private final boolean isEditVisibleAssetType;
    private Spinner mNetworkSp;
    private NetworkSpinnerAdapter mNetworkAdapter;
    private ArrayList<NetworkInfo> mSpinnerNetworks;

    private ActivityResultLauncher<Intent> mAddAssetActivityResultLauncher;

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        if (mUserAssetModel == null) return;
        mSelectedNetwork = mSpinnerNetworks.get(position);
        mUserAssetModel.fetchAssets(mNftsOnly, mSelectedNetwork);
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {}

    public interface DismissListener {
        void onDismiss(boolean isAssetsListChanged);
    }

    public static EditVisibleAssetsBottomSheetDialogFragment newInstance(
            WalletCoinAdapter.AdapterType type, boolean nftsOnly) {
        return new EditVisibleAssetsBottomSheetDialogFragment(type, nftsOnly);
    }

    private EditVisibleAssetsBottomSheetDialogFragment(
            WalletCoinAdapter.AdapterType type, boolean nftsOnly) {
        mType = type;
        mNftsOnly = nftsOnly;
        isEditVisibleAssetType = mType == WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST;
    }

    // TODO (Wengling): add an interface for getting services that can be shared between activity
    // and fragments
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

    public void setSelectedNetwork(NetworkInfo selectedNetwork) {
        assert selectedNetwork != null;
        mSelectedNetwork = selectedNetwork;
    }

    public void setOnAssetClickListener(
            OnEditVisibleItemClickListener onEditVisibleItemClickListener) {
        mOnEditVisibleItemClickListener = onEditVisibleItemClickListener;
    }

    public void setDismissListener(DismissListener dismissListener) {
        mDismissListener = dismissListener;
    }

    @Override
    public void show(FragmentManager manager, String tag) {
        try {
            EditVisibleAssetsBottomSheetDialogFragment fragment =
                    (EditVisibleAssetsBottomSheetDialogFragment) manager.findFragmentByTag(
                            EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e(TAG, "Error when showing EditVisibleAssetsBottomSheetDialogFragment.", e);
        }
    }

    @Override
    public void locked() {
        dismiss();
    }

    @Override
    @NonNull
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            mUserAssetModel = mWalletModel.getCryptoModel().createUserAssetModel(mType);
            mSelectedNetwork = JavaUtils.safeVal(
                    mSelectedNetwork, NetworkUtils.getAllNetworkOption(requireContext()));
            mUserAssetModel.fetchAssets(mNftsOnly, mSelectedNetwork);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "Error during dialog creation.", e);
        }

        KeyringService keyringService = getKeyringService();
        assert keyringService != null;
        mKeyringServiceObserver = new KeyringServiceObserverImpl(this);
        keyringService.addObserver(mKeyringServiceObserver);

        Dialog dialog = super.onCreateDialog(savedInstanceState);
        dialog.setOnShowListener(dialogInterface -> {
            BottomSheetDialog bottomSheetDialog = (BottomSheetDialog) dialogInterface;
            setupFullHeight(bottomSheetDialog);
        });
        return dialog;
    }

    @Override
    public void onDismiss(@NonNull DialogInterface dialog) {
        super.onDismiss(dialog);
        if (mDismissListener != null) {
            mDismissListener.onDismiss(mIsAssetsListChanged);
        }
        if (mKeyringServiceObserver != null) {
            mKeyringServiceObserver.close();
            mKeyringServiceObserver = null;
        }
    }

    private void setupFullHeight(BottomSheetDialog bottomSheetDialog) {
        FrameLayout bottomSheet = bottomSheetDialog.findViewById(R.id.design_bottom_sheet);
        if (bottomSheet != null) {
            BottomSheetBehavior<FrameLayout> behavior = BottomSheetBehavior.from(bottomSheet);
            ViewGroup.LayoutParams layoutParams = bottomSheet.getLayoutParams();

            int windowHeight = getWindowHeight();
            if (layoutParams != null) {
                layoutParams.height = windowHeight;
            }
            bottomSheet.setLayoutParams(layoutParams);
            behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
        }
    }

    private int getWindowHeight() {
        // Calculate window height for fullscreen use
        DisplayMetrics displayMetrics = new DisplayMetrics();
        requireActivity().getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        return displayMetrics.heightPixels;
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        final View view =
                inflater.inflate(R.layout.edit_visible_assets_bottom_sheet, container, false);

        Button saveAssets = view.findViewById(R.id.saveAssets);
        TextView addCustomAsset = view.findViewById(R.id.add_custom_asset);
        addCustomAsset.setText(
                mNftsOnly ? R.string.wallet_add_nft : R.string.wallet_add_custom_asset);
        if (mType == WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST) {
            addCustomAsset.setOnClickListener(clickView -> showAddAssetActivity());
            saveAssets.setOnClickListener(clickView -> dismiss());
        } else {
            saveAssets.setVisibility(View.GONE);
            addCustomAsset.setVisibility(View.GONE);
        }

        if (mType == WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST) {
            mNetworkSp = view.findViewById(R.id.edit_visible_network_spinner);
            mNetworkAdapter = new NetworkSpinnerAdapter(requireContext(), Collections.emptyList());
            mNetworkSp.setAdapter(mNetworkAdapter);
            AndroidUtils.show(mNetworkSp);
            mNetworkSp.setOnItemSelectedListener(this);
        }
        return view;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        setUpObservers();
    }

    private void setUpObservers() {
        if (JavaUtils.anyNull(mUserAssetModel, mWalletModel)) return;
        LiveDataUtil.observeOnce(
                mWalletModel.getCryptoModel().getNetworkModel().mCryptoNetworks, networkInfos -> {
                    mCryptoNetworks = networkInfos;
                    mUserAssetModel.mAssetsResult.observe(getViewLifecycleOwner(), assetsResult -> {
                        setUpAssetsList(getView(), assetsResult.tokens, assetsResult.userAssets);
                    });
                    if (mNetworkAdapter != null) {
                        mSpinnerNetworks = new ArrayList<>(networkInfos);
                        mSpinnerNetworks.add(0, NetworkUtils.getAllNetworkOption(requireContext()));
                        mNetworkAdapter.setNetworks(mSpinnerNetworks);
                        int selectedNetworkIndex =
                                IntStream.range(0, mSpinnerNetworks.size())
                                        .filter(i
                                                -> mSpinnerNetworks.get(i).chainId.equals(
                                                        mSelectedNetwork.chainId))
                                        .findFirst()
                                        .orElse(0);
                        mNetworkSp.setSelection(selectedNetworkIndex, false);
                    }
                });
    }

    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);

        // Pass @{code ActivityResultRegistry} reference explicitly to avoid crash
        // https://github.com/brave/brave-browser/issues/31882
        mAddAssetActivityResultLauncher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                ((BraveWalletBaseActivity) requireActivity()).getActivityResultRegistry(),
                result -> {
                    if (result.getResultCode() == Activity.RESULT_OK) {
                        Intent intent = result.getData();
                        if (intent == null) {
                            return;
                        }
                        final BlockchainToken token =
                                WalletUtils.getBlockchainTokenFromIntent(intent);
                        final NetworkInfo networkInfo =
                                WalletUtils.getNetworkInfoFromIntent(intent);
                        if (token == null || networkInfo == null) {
                            return;
                        }

                        boolean isDuplicateToken = false;
                        for (WalletListItemModel item : mWalletCoinAdapter.getCheckedAssets()) {
                            // We can have multiple ERC721 or Solana NFTs with the same name
                            if (!item.isNft()
                                    && (item.getTitle().equals(token.name)
                                            || item.getSubTitle().equals(token.symbol))) {
                                isDuplicateToken = true;
                                break;
                            }
                        }
                        if (!isDuplicateToken) {
                            WalletListItemModel itemModel =
                                    new WalletListItemModel(R.drawable.ic_eth, token.name,
                                            token.symbol, token.tokenId, "", "");
                            itemModel.setBlockchainToken(token);
                            itemModel.setIconPath(token.logo);
                            itemModel.setAssetNetwork(networkInfo);

                            itemModel.isVisible(true);
                            mWalletCoinAdapter.addItem(itemModel);
                            mIsAssetsListChanged = true;
                        }
                    }
                });
    }

    private void showAddAssetActivity() {
        Intent addAssetIntent = AddAssetActivity.getIntent(requireContext(), mNftsOnly);
        mAddAssetActivityResultLauncher.launch(addAssetIntent);
    }

    private void setUpAssetsList(
            View view, List<BlockchainToken> tokens, List<BlockchainToken> userSelectedTokens) {
        HashSet<String> selectedTokensSymbols = new HashSet<>();
        if (!mNftsOnly) {
            for (BlockchainToken userSelectedToken : userSelectedTokens) {
                selectedTokensSymbols.add(Utils.tokenToString(userSelectedToken));
            }
        }
        RecyclerView rvAssets = view.findViewById(R.id.rvAssets);
        mWalletCoinAdapter = new WalletCoinAdapter(mType);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();
        for (int i = 0; i < tokens.size(); i++) {
            BlockchainToken token = tokens.get(i);

            NetworkInfo assetNetwork =
                    NetworkUtils.findNetwork(mCryptoNetworks, token.chainId, token.coin);
            if (assetNetwork == null) {
                Log.e(TAG,
                        String.format("Asset network for token with chain Id %s was null.",
                                token.chainId));
                continue;
            }
            String subtitle = !isEditVisibleAssetType
                    ? token.symbol
                    : getString(R.string.brave_wallet_portfolio_asset_network_description,
                            token.symbol, assetNetwork.chainName);

            WalletListItemModel itemModel = new WalletListItemModel(
                    Utils.getCoinIcon(token.coin), token.name, subtitle, token.tokenId, "", "");
            itemModel.setBlockchainToken(token);
            itemModel.setAssetNetwork(assetNetwork);
            itemModel.setBrowserResourcePath(tokensPath);
            itemModel.setIconPath("file://" + tokensPath + "/" + token.logo);
            if (mNftsOnly) {
                itemModel.isVisible(token.visible);
            } else {
                itemModel.isVisible(selectedTokensSymbols.contains(Utils.tokenToString(token)));
            }

            walletListItemModelList.add(itemModel);
        }
        walletListItemModelList.sort(
                (item1, item2) -> compare(item2.isVisible(), item1.isVisible()));
        mWalletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        mWalletCoinAdapter.setOnWalletListItemClick(this);
        mWalletCoinAdapter.setWalletListItemType(Utils.ASSET_ITEM);
        rvAssets.setAdapter(mWalletCoinAdapter);
        rvAssets.setLayoutManager(new LinearLayoutManager(getActivity()));
        SearchView searchView = view.findViewById(R.id.searchView);
        searchView.setQueryHint(getText(R.string.search_tokens));
        searchView.setIconified(false);
        searchView.clearFocus();
        searchView.setOnQueryTextListener(new SearchView.OnQueryTextListener() {
            @Override
            public boolean onQueryTextSubmit(String query) {
                if (mWalletCoinAdapter != null) {
                    mWalletCoinAdapter.filter(query, !isEditVisibleAssetType);
                }

                return true;
            }

            @Override
            public boolean onQueryTextChange(String newText) {
                if (mWalletCoinAdapter != null) {
                    mWalletCoinAdapter.filter(newText, !isEditVisibleAssetType);
                }

                return true;
            }
        });
    }

    @Override
    public void onClick(View view) {
        dismiss();
    }

    @Override
    public void onAssetClick(BlockchainToken token) {
        List<WalletListItemModel> checkedAssets = mWalletCoinAdapter.getCheckedAssets();
        if (mOnEditVisibleItemClickListener != null) {
            mOnEditVisibleItemClickListener.onAssetClick(checkedAssets.get(0));
        }
        dismiss();
    }

    @Override
    public void onTrashIconClick(WalletListItemModel walletListItemModel) {
        AlertDialog.Builder builder = new AlertDialog.Builder(requireContext());
        builder.setTitle(R.string.wallet_remove_custom_asset);
        builder.setMessage(String.format(getString(R.string.wallet_remove_custom_asset_desc),
                walletListItemModel.getBlockchainToken().name,
                walletListItemModel.getBlockchainToken().symbol));

        builder.setPositiveButton(R.string.wallet_remove_custom_asset_yes, (dialog, id) -> {
            BraveWalletService braveWalletService = getBraveWalletService();
            assert braveWalletService != null;
            braveWalletService.removeUserAsset(
                    walletListItemModel.getBlockchainToken(), (success) -> {
                        if (success) {
                            mWalletCoinAdapter.removeItem(walletListItemModel);
                            mIsAssetsListChanged = true;
                        }
                    });
        });
        builder.setNegativeButton(R.string.wallet_remove_custom_asset_no, null);
        AlertDialog alert = builder.create();
        alert.show();
    }

    @Override
    public void onMaybeShowTrashButton(
            WalletListItemModel walletListItemModel, ImageView trashButton) {
        assert walletListItemModel.getAssetNetwork() != null : "Network should not be null";
        // Prevent NPE of network, issue:#31303
        if (mType != WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST
                || walletListItemModel.getAssetNetwork() == null) {
            return;
        }

        TokenUtils.isCustomToken(getBlockchainRegistry(), walletListItemModel.getAssetNetwork(),
                walletListItemModel.getBlockchainToken().coin,
                walletListItemModel.getBlockchainToken(), isCustom -> {
                    if (!isCustom) return;

                    trashButton.setVisibility(View.VISIBLE);
                });
    }

    @Override
    public void onAssetCheckedChanged(
            WalletListItemModel walletListItemModel, CheckBox assetCheck, boolean isChecked) {
        assert walletListItemModel.getAssetNetwork() != null : "Network should not be null";
        // Prevent NPE of network, issue:#31303
        if (mType != WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST
                || walletListItemModel.getAssetNetwork() == null) {
            return;
        }

        BlockchainToken thisToken = walletListItemModel.getBlockchainToken();
        final BraveWalletService braveWalletService = getBraveWalletService();
        if (braveWalletService == null) {
            Log.e(TAG, "BraveWalletService was null.");
            return;
        }

        final BlockchainRegistry blockchainRegistry = getBlockchainRegistry();
        if (blockchainRegistry == null) {
            Log.e(TAG, "BlockchainRegistry was null.");
            return;
        }

        TokenUtils.isCustomToken(blockchainRegistry, walletListItemModel.getAssetNetwork(),
                walletListItemModel.getBlockchainToken().coin, thisToken, isCustom -> {
                    // Only show add asset dialog on click when:
                    //    1. It is an ERC721 token
                    //    2. It is a token listed in Registry
                    //    3. It is not user added (i.e. doesn't have a token id)
                    if (thisToken.isErc721 && !isCustom
                            && (thisToken.tokenId == null || thisToken.tokenId.trim().isEmpty())) {
                        showAddAssetActivity();
                        walletListItemModel.isVisible(
                                false); // The added token is different from the listed one
                        itemCheckboxConsistency(walletListItemModel, assetCheck, isChecked);
                    } else {
                        if (!isCustom) {
                            if (isChecked) {
                                braveWalletService.addUserAsset(thisToken, (success) -> {
                                    if (success) {
                                        walletListItemModel.isVisible(true);
                                    }
                                    itemCheckboxConsistency(walletListItemModel, assetCheck, true);
                                });
                            } else {
                                braveWalletService.removeUserAsset(thisToken, (success) -> {
                                    if (success) {
                                        walletListItemModel.isVisible(false);
                                    }
                                    itemCheckboxConsistency(walletListItemModel, assetCheck, false);
                                });
                            }
                        } else {
                            braveWalletService.setUserAssetVisible(
                                    thisToken, isChecked, success -> {
                                        if (success) {
                                            walletListItemModel.isVisible(isChecked);
                                        }
                                        itemCheckboxConsistency(
                                                walletListItemModel, assetCheck, isChecked);
                                    });
                        }
                        mIsAssetsListChanged = true;
                    }
                });
    }

    private void itemCheckboxConsistency(
            WalletListItemModel walletListItemModel, CheckBox assetCheck, boolean isChecked) {
        if (isChecked != walletListItemModel.isVisible()) {
            assetCheck.setTag("noOnClickListener");
            assetCheck.setChecked(walletListItemModel.isVisible());
        }
    }

    public interface OnEditVisibleItemClickListener {
        default void onAssetClick(WalletListItemModel asset) {}
    }
}
