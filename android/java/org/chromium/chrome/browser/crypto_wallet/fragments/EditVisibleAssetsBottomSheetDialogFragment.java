/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.SearchView;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.UserAssetModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
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
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
public class EditVisibleAssetsBottomSheetDialogFragment extends BottomSheetDialogFragment
        implements View.OnClickListener, OnWalletListItemClick, KeyringServiceObserverImplDelegate {
    public static final String TAG_FRAGMENT =
            EditVisibleAssetsBottomSheetDialogFragment.class.getName();
    private WalletCoinAdapter walletCoinAdapter;
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
    private boolean isEditVisibleAssetType;

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

    private AssetRatioService getAssetRatioService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getAssetRatioService();
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

    private JsonRpcService getJsonRpcService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getJsonRpcService();
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
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            mUserAssetModel = mWalletModel.getCryptoModel().createUserAssetModel(mType);
            mUserAssetModel.fetchAssets(mNftsOnly, mSelectedNetwork);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "Error during dialog creation.", e);
        }

        KeyringService keyringService = getKeyringService();
        assert keyringService != null;
        mKeyringServiceObserver = new KeyringServiceObserverImpl(this);
        keyringService.addObserver(mKeyringServiceObserver);

        Dialog dialog = super.onCreateDialog(savedInstanceState);
        dialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface dialogInterface) {
                BottomSheetDialog bottomSheetDialog = (BottomSheetDialog) dialogInterface;
                setupFullHeight(bottomSheetDialog);
            }
        });
        return dialog;
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
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
        FrameLayout bottomSheet =
                (FrameLayout) bottomSheetDialog.findViewById(R.id.design_bottom_sheet);
        BottomSheetBehavior behavior = BottomSheetBehavior.from(bottomSheet);
        ViewGroup.LayoutParams layoutParams = bottomSheet.getLayoutParams();

        int windowHeight = getWindowHeight();
        if (layoutParams != null) {
            layoutParams.height = windowHeight;
        }
        bottomSheet.setLayoutParams(layoutParams);
        behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
    }

    private int getWindowHeight() {
        // Calculate window height for fullscreen use
        DisplayMetrics displayMetrics = new DisplayMetrics();
        ((Activity) getContext()).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        return displayMetrics.heightPixels;
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        @SuppressLint("InflateParams")
        final View view = LayoutInflater.from(getContext())
                                  .inflate(R.layout.edit_visible_assets_bottom_sheet, null);

        Button saveAssets = view.findViewById(R.id.saveAssets);
        TextView addCustomAsset = view.findViewById(R.id.add_custom_asset);
        addCustomAsset.setText(
                mNftsOnly ? R.string.wallet_add_nft : R.string.wallet_add_custom_asset);
        if (mType == WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST) {
            // TODO(pav): Revert this after adding network selector in add custom asset
            if (NetworkUtils.isAllNetwork(mSelectedNetwork)) {
                AndroidUtils.gone(addCustomAsset);
            } else {
                addCustomAsset.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View clickView) {
                        showAddAssetDialog();
                    }
                });
            }
            saveAssets.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View clickView) {
                    dismiss();
                }
            });
        } else {
            saveAssets.setVisibility(View.GONE);
            addCustomAsset.setVisibility(View.GONE);
        }

        if (mType == WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST) {
            assert mSelectedNetwork != null;
            Button networkNameBtn = view.findViewById(R.id.edit_visible_btn_network);
            AndroidUtils.show(networkNameBtn);
            networkNameBtn.setText(Utils.getShortNameOfNetwork(mSelectedNetwork.chainName));
            networkNameBtn.setOnLongClickListener(v -> {
                Toast.makeText(requireContext(), mSelectedNetwork.chainName, Toast.LENGTH_SHORT)
                        .show();
                return true;
            });
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
                });
    }

    private void showAddAssetDialog() {
        Dialog dialog = new Dialog(getActivity());
        dialog.setContentView(R.layout.brave_wallet_add_custom_asset);
        dialog.show();

        TextView title = dialog.findViewById(R.id.add_asset_dialog_title);
        title.setText(mNftsOnly ? R.string.wallet_add_nft : R.string.wallet_add_custom_asset);

        if (mSelectedNetwork.coin == CoinType.ETH) {
            LinearLayout advancedSection = dialog.findViewById(R.id.advanced_section);
            if (mNftsOnly) {
                advancedSection.setVisibility(View.VISIBLE);
            } else {
                advancedSection.setVisibility(View.GONE);
            }
        } else {
            if (mSelectedNetwork.coin == CoinType.SOL) {
                dialog.findViewById(R.id.token_decimals_title)
                        .setVisibility(mNftsOnly ? View.GONE : View.VISIBLE);
                dialog.findViewById(R.id.token_decimals)
                        .setVisibility(mNftsOnly ? View.GONE : View.VISIBLE);
                ((TextView) dialog.findViewById(R.id.token_contract_address_title))
                        .setText(getString(mNftsOnly
                                        ? R.string.wallet_add_custom_asset_token_address
                                        : R.string.wallet_add_custom_asset_token_contract_address));
            }
        }

        Button cancel = dialog.findViewById(R.id.cancel);
        cancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.dismiss();
            }
        });
        Button add = dialog.findViewById(R.id.add);
        filterAddCustomAssetTextWatcher.setDialog(dialog);
        EditText tokenNameEdit = dialog.findViewById(R.id.token_name);
        EditText tokenContractAddressEdit = dialog.findViewById(R.id.token_contract_address);
        EditText tokenSymbolEdit = dialog.findViewById(R.id.token_symbol);
        EditText tokenDecimalsEdit = dialog.findViewById(R.id.token_decimals);
        EditText tokenIdEdit = dialog.findViewById(R.id.token_id);
        tokenNameEdit.addTextChangedListener(filterAddCustomAssetTextWatcher);
        tokenContractAddressEdit.addTextChangedListener(filterAddCustomAssetTextWatcher);
        tokenSymbolEdit.addTextChangedListener(filterAddCustomAssetTextWatcher);
        tokenDecimalsEdit.addTextChangedListener(filterAddCustomAssetTextWatcher);
        tokenIdEdit.addTextChangedListener(filterAddCustomAssetTextWatcher);
        add.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                BraveWalletService braveWalletService = getBraveWalletService();
                assert braveWalletService != null;
                BlockchainToken token = new BlockchainToken();
                token.contractAddress = tokenContractAddressEdit.getText().toString();
                token.name = tokenNameEdit.getText().toString();
                token.logo = "";
                String tokenId = tokenIdEdit.getText().toString();
                token.tokenId = Utils.toHex(tokenId);
                boolean isErc721 = tokenId != null && !tokenId.trim().isEmpty();
                token.isErc20 = !isErc721;
                token.isErc721 = isErc721;
                token.isNft = isErc721;
                token.symbol = tokenSymbolEdit.getText().toString();
                token.decimals = mSelectedNetwork.decimals;
                token.chainId = mSelectedNetwork.chainId;
                token.coin = mSelectedNetwork.coin;
                try {
                    token.decimals = Integer.valueOf(tokenDecimalsEdit.getText().toString());
                } catch (NumberFormatException exc) {
                }
                if (mSelectedNetwork.coin == CoinType.SOL) {
                    token.isNft = mNftsOnly;
                    if (mNftsOnly) {
                        token.decimals = 0;
                    }
                }
                token.visible = true;

                braveWalletService.addUserAsset(token, success -> {
                    if (success) {
                        WalletListItemModel itemModel = new WalletListItemModel(
                                R.drawable.ic_eth, token.name, token.symbol, token.tokenId, "", "");
                        itemModel.setBlockchainToken(token);
                        itemModel.setIconPath(token.logo);

                        itemModel.setIsUserSelected(true);
                        walletCoinAdapter.addItem(itemModel);
                        mIsAssetsListChanged = true;
                    }
                    dialog.dismiss();
                });
            }
        });
    }

    private TextWatcherImpl filterAddCustomAssetTextWatcher = new TextWatcherImpl();

    private class TextWatcherImpl implements TextWatcher {
        private Dialog mDialog;
        private boolean selfChange;
        private EditText tokenNameEdit;
        private EditText tokenContractAddressEdit;
        private EditText tokenSymbolEdit;
        private EditText tokenDecimalsEdit;
        private EditText tokenIdEdit;
        private Button addButton;

        public void setDialog(Dialog dialog) {
            mDialog = dialog;
            selfChange = false;
            tokenNameEdit = mDialog.findViewById(R.id.token_name);
            tokenContractAddressEdit = mDialog.findViewById(R.id.token_contract_address);
            tokenSymbolEdit = mDialog.findViewById(R.id.token_symbol);
            tokenDecimalsEdit = mDialog.findViewById(R.id.token_decimals);
            tokenIdEdit = mDialog.findViewById(R.id.token_id);
            addButton = mDialog.findViewById(R.id.add);
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            if (selfChange) return;
            String contractAddress = tokenContractAddressEdit.getText().toString();
            String contractAddressTrimmed = contractAddress.trim();
            if (!contractAddress.equals(contractAddressTrimmed)) {
                // update the contractAddress and process it within the next onTextChanged pass
                tokenContractAddressEdit.setText(
                        contractAddressTrimmed, TextView.BufferType.EDITABLE);
                return;
            }
            String tokenName = tokenNameEdit.getText().toString();
            String tokenSymbol = tokenSymbolEdit.getText().toString();

            tokenIdEdit.setEnabled(true);
            addButton.setEnabled(false);

            boolean isDuplicateToken = false;
            for (WalletListItemModel item : walletCoinAdapter.getCheckedAssets()) {
                // We can have multiple ERC721 or Solana NFTs with the same name
                if (!item.isNft()
                        && (item.getTitle().equals(tokenName)
                                || item.getSubTitle().equals(tokenSymbol))) {
                    isDuplicateToken = true;
                    break;
                }
            }
            if (isDuplicateToken) {
                return;
            }

            AssetRatioService assetRatioService = getAssetRatioService();
            // Do not assert here, service can be null when backed from dialog
            if (assetRatioService != null && !contractAddress.isEmpty()
                    && mSelectedNetwork.coin == CoinType.ETH) {
                assetRatioService.getTokenInfo(contractAddress, token -> {
                    if (token != null) {
                        selfChange = true;
                        tokenNameEdit.setText(token.name, TextView.BufferType.EDITABLE);
                        tokenSymbolEdit.setText(token.symbol, TextView.BufferType.EDITABLE);
                        tokenDecimalsEdit.setText(
                                String.valueOf(token.decimals), TextView.BufferType.EDITABLE);
                        if (!token.isErc721) tokenIdEdit.setEnabled(false);
                        selfChange = false;
                        if (!mNftsOnly
                                || (token.isErc721 && !tokenIdEdit.getText().toString().isEmpty())
                                || !token.isErc721) {
                            addButton.setEnabled(true);
                        }
                    }
                });
            }

            if (tokenName.isEmpty() || tokenSymbol.isEmpty()
                    || (mSelectedNetwork.coin == CoinType.ETH
                            && ((mNftsOnly && tokenIdEdit.getText().toString().isEmpty())
                                    || tokenDecimalsEdit.getText().toString().isEmpty()))) {
                return;
            }

            addButton.setEnabled(true);
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
    };

    private void setUpAssetsList(
            View view, List<BlockchainToken> tokens, List<BlockchainToken> userSelectedTokens) {
        HashSet<String> selectedTokensSymbols = new HashSet<String>();
        for (BlockchainToken userSelectedToken : userSelectedTokens) {
            selectedTokensSymbols.add(Utils.tokenToString(userSelectedToken));
        }
        RecyclerView rvAssets = view.findViewById(R.id.rvAssets);
        walletCoinAdapter = new WalletCoinAdapter(mType);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();
        for (int i = 0; i < tokens.size(); i++) {
            BlockchainToken token = tokens.get(i);

            NetworkInfo assetNetwork = NetworkUtils.findNetwork(mCryptoNetworks, token.chainId);
            String subtitle = !isEditVisibleAssetType || assetNetwork == null
                    ? token.symbol
                    : getString(R.string.brave_wallet_portfolio_asset_network_description,
                            token.symbol, assetNetwork.chainName);

            WalletListItemModel itemModel = new WalletListItemModel(
                    Utils.getCoinIcon(token.coin), token.name, subtitle, token.tokenId, "", "");
            itemModel.setBlockchainToken(token);
            itemModel.setAssetNetwork(assetNetwork);
            itemModel.setBrowserResourcePath(tokensPath);
            itemModel.setIconPath("file://" + tokensPath + "/" + token.logo);
            itemModel.setIsUserSelected(selectedTokensSymbols.contains(Utils.tokenToString(token)));
            walletListItemModelList.add(itemModel);
        }
        walletListItemModelList.sort(
                (item1, item2)
                        -> Boolean.compare(item2.getIsUserSelected(), item1.getIsUserSelected()));
        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletCoinAdapter.setOnWalletListItemClick(this);
        walletCoinAdapter.setWalletListItemType(Utils.ASSET_ITEM);
        rvAssets.setAdapter(walletCoinAdapter);
        rvAssets.setLayoutManager(new LinearLayoutManager(getActivity()));
        SearchView searchView = (SearchView) view.findViewById(R.id.searchView);
        searchView.setQueryHint(getText(R.string.search_tokens));
        searchView.setIconified(false);
        searchView.clearFocus();
        searchView.setOnQueryTextListener(new SearchView.OnQueryTextListener() {
            @Override
            public boolean onQueryTextSubmit(String query) {
                if (walletCoinAdapter != null) {
                    walletCoinAdapter.filter(query, !isEditVisibleAssetType);
                }

                return true;
            }

            @Override
            public boolean onQueryTextChange(String newText) {
                if (walletCoinAdapter != null) {
                    walletCoinAdapter.filter(newText, !isEditVisibleAssetType);
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
        List<WalletListItemModel> checkedAssets = walletCoinAdapter.getCheckedAssets();
        if (mOnEditVisibleItemClickListener != null) {
            mOnEditVisibleItemClickListener.onAssetClick(checkedAssets.get(0));
        }
        dismiss();
    }

    @Override
    public void onTrashIconClick(WalletListItemModel walletListItemModel) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        builder.setTitle(R.string.wallet_remove_custom_asset);
        builder.setMessage(String.format(getString(R.string.wallet_remove_custom_asset_desc),
                walletListItemModel.getBlockchainToken().name,
                walletListItemModel.getBlockchainToken().symbol));

        builder.setPositiveButton(
                R.string.wallet_remove_custom_asset_yes, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int id) {
                        BraveWalletService braveWalletService = getBraveWalletService();
                        assert braveWalletService != null;
                        braveWalletService.removeUserAsset(
                                walletListItemModel.getBlockchainToken(), (success) -> {
                                    if (success) {
                                        walletCoinAdapter.removeItem(walletListItemModel);
                                        mIsAssetsListChanged = true;
                                    }
                                });
                    }
                });
        builder.setNegativeButton(R.string.wallet_remove_custom_asset_no, null);
        AlertDialog alert = builder.create();
        alert.show();
    }

    @Override
    public void onMaybeShowTrashButton(
            WalletListItemModel walletListItemModel, ImageView trashButton) {
        if (mType != WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST) return;

        JsonRpcService jsonRpcService = getJsonRpcService();
        if (jsonRpcService == null) return;
        jsonRpcService.getNetwork(
                walletListItemModel.getBlockchainToken().coin, null, selectedNetwork -> {
                    TokenUtils.isCustomToken(getBlockchainRegistry(), selectedNetwork,
                            selectedNetwork.coin, walletListItemModel.getBlockchainToken(),
                            isCustom -> {
                                if (!isCustom) return;

                                trashButton.setVisibility(View.VISIBLE);
                            });
                });
    }

    @Override
    public void onAssetCheckedChanged(
            WalletListItemModel walletListItemModel, CheckBox assetCheck, boolean isChecked) {
        if (mType != WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST) return;
        JsonRpcService jsonRpcService = getJsonRpcService();
        if (jsonRpcService == null) return;

        BlockchainToken thisToken = walletListItemModel.getBlockchainToken();
        TokenUtils.isCustomToken(getBlockchainRegistry(), walletListItemModel.getAssetNetwork(),
                walletListItemModel.getAssetNetwork().coin, thisToken, isCustom -> {
                    // Only show add asset dialog on click when:
                    //    1. It is an ERC721 token
                    //    2. It is a token listed in Registry
                    //    3. It is not user added (i.e. doesn't have a token id)
                    if (thisToken.isErc721 && !isCustom
                            && (thisToken.tokenId == null || thisToken.tokenId.trim().isEmpty())) {
                        showAddAssetDialog();
                        walletListItemModel.setIsUserSelected(
                                false); // The added token is different from the listed one
                        itemCheckboxConsistency(walletListItemModel, assetCheck, isChecked);
                    } else {
                        BraveWalletService braveWalletService = getBraveWalletService();
                        // TODO: all the asserts need to be removed. Shall do proper
                        // error handling instead.
                        assert braveWalletService != null;
                        if (!isCustom) {
                            if (isChecked) {
                                braveWalletService.addUserAsset(thisToken, (success) -> {
                                    if (success) {
                                        walletListItemModel.setIsUserSelected(true);
                                    }
                                    itemCheckboxConsistency(
                                            walletListItemModel, assetCheck, isChecked);
                                });
                            } else {
                                braveWalletService.removeUserAsset(thisToken, (success) -> {
                                    if (success) {
                                        walletListItemModel.setIsUserSelected(false);
                                    }
                                    itemCheckboxConsistency(
                                            walletListItemModel, assetCheck, isChecked);
                                });
                            }
                        } else {
                            braveWalletService.setUserAssetVisible(
                                    thisToken, isChecked, success -> {
                                        if (success) {
                                            walletListItemModel.setIsUserSelected(isChecked);
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
        if (isChecked != walletListItemModel.getIsUserSelected()) {
            assetCheck.setTag("noOnClickListener");
            assetCheck.setChecked(walletListItemModel.getIsUserSelected());
        }
    }

    public interface OnEditVisibleItemClickListener {
        default void onAssetClick(WalletListItemModel asset) {}
    }
}
