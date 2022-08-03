/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
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
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;

public class EditVisibleAssetsBottomSheetDialogFragment extends BottomSheetDialogFragment
        implements View.OnClickListener, OnWalletListItemClick, KeyringServiceObserver {
    public static final String TAG_FRAGMENT =
            EditVisibleAssetsBottomSheetDialogFragment.class.getName();
    private WalletCoinAdapter walletCoinAdapter;
    private WalletCoinAdapter.AdapterType mType;
    private NetworkInfo mSelectedNetwork;
    private DismissListener mDismissListener;
    private Boolean mIsAssetsListChanged;
    private static final String TAG = "EditVisibleAssetsBottomSheetDialogFragment";

    public interface DismissListener {
        void onDismiss(Boolean isAssetsListChanged);
    }

    public static EditVisibleAssetsBottomSheetDialogFragment newInstance(
            WalletCoinAdapter.AdapterType type) {
        return new EditVisibleAssetsBottomSheetDialogFragment(type);
    }

    private EditVisibleAssetsBottomSheetDialogFragment(WalletCoinAdapter.AdapterType type) {
        mType = type;
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
            Log.e("EditVisibleAssetsBottomSheetDialogFragment", e.getMessage());
        }
    }

    @Override
    public void locked() {
        dismiss();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
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

    @Override
    public void setupDialog(@NonNull Dialog dialog, int style) {
        super.setupDialog(dialog, style);

        @SuppressLint("InflateParams")
        final View view = LayoutInflater.from(getContext())
                                  .inflate(R.layout.edit_visible_assets_bottom_sheet, null);

        dialog.setContentView(view);
        ViewParent parent = view.getParent();
        ((View) parent).getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
        Button saveAssets = view.findViewById(R.id.saveAssets);
        TextView addCustomAsset = view.findViewById(R.id.add_custom_asset);
        if (mType == WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST) {
            saveAssets.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View clickView) {
                    dismiss();
                }
            });
            addCustomAsset.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View clickView) {
                    showAddAssetDialog();
                }
            });
        } else {
            saveAssets.setVisibility(View.GONE);
            addCustomAsset.setVisibility(View.GONE);
        }

        BlockchainRegistry blockchainRegistry = getBlockchainRegistry();
        if (blockchainRegistry != null) {
            BraveWalletService braveWalletService = getBraveWalletService();
            assert braveWalletService != null;
            if (mType == WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST) {
                assert mSelectedNetwork != null;
                TokenUtils.getUserAssetsFiltered(braveWalletService, mSelectedNetwork,
                        mSelectedNetwork.coin, TokenUtils.TokenType.ALL, userAssets -> {
                            TokenUtils.getAllTokensFiltered(braveWalletService, blockchainRegistry,
                                    mSelectedNetwork, mSelectedNetwork.coin,
                                    TokenUtils.TokenType.ALL,
                                    tokens -> { setUpAssetsList(view, tokens, userAssets); });
                        });
            } else if (mType == WalletCoinAdapter.AdapterType.SEND_ASSETS_LIST) {
                assert mSelectedNetwork != null;
                TokenUtils.getUserAssetsFiltered(braveWalletService, mSelectedNetwork,
                        mSelectedNetwork.coin, TokenUtils.TokenType.ALL,
                        tokens -> { setUpAssetsList(view, tokens, new BlockchainToken[0]); });
            } else if (mType == WalletCoinAdapter.AdapterType.SWAP_TO_ASSETS_LIST
                    || mType == WalletCoinAdapter.AdapterType.SWAP_FROM_ASSETS_LIST) {
                assert mSelectedNetwork != null;
                TokenUtils.getAllTokensFiltered(braveWalletService, blockchainRegistry,
                        mSelectedNetwork, mSelectedNetwork.coin, TokenUtils.TokenType.ERC20,
                        tokens -> { setUpAssetsList(view, tokens, new BlockchainToken[0]); });
            } else if (mType == WalletCoinAdapter.AdapterType.BUY_ASSETS_LIST) {
                TokenUtils.getBuyTokensFiltered(blockchainRegistry, mSelectedNetwork,
                        TokenUtils.TokenType.ERC20,
                        tokens -> { setUpAssetsList(view, tokens, new BlockchainToken[0]); });
            }
        }
        KeyringService keyringService = getKeyringService();
        assert keyringService != null;
        keyringService.addObserver(this);
    }

    private void showAddAssetDialog() {
        Dialog dialog = new Dialog(getActivity());
        dialog.setContentView(R.layout.brave_wallet_add_custom_asset);
        dialog.show();

        LinearLayout advancedSection = dialog.findViewById(R.id.advanced_section);
        TextView advancedTitle = dialog.findViewById(R.id.advanced_title);
        advancedTitle.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (advancedSection.getVisibility() == View.GONE)
                    advancedSection.setVisibility(View.VISIBLE);
                else
                    advancedSection.setVisibility(View.GONE);
            }
        });

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
                token.symbol = tokenSymbolEdit.getText().toString();
                token.decimals = mSelectedNetwork.decimals;
                token.chainId = mSelectedNetwork.chainId;
                token.coin = mSelectedNetwork.coin;
                try {
                    token.decimals = Integer.valueOf(tokenDecimalsEdit.getText().toString());
                } catch (NumberFormatException exc) {
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
        Dialog mDialog;
        boolean selfChange;

        public void setDialog(Dialog dialog) {
            mDialog = dialog;
            selfChange = false;
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            if (selfChange) return;
            EditText tokenNameEdit = mDialog.findViewById(R.id.token_name);
            EditText tokenContractAddressEdit = mDialog.findViewById(R.id.token_contract_address);
            EditText tokenSymbolEdit = mDialog.findViewById(R.id.token_symbol);
            EditText tokenDecimalsEdit = mDialog.findViewById(R.id.token_decimals);
            EditText tokenIdEdit = mDialog.findViewById(R.id.token_id);
            tokenIdEdit.setEnabled(true);
            Button addButton = mDialog.findViewById(R.id.add);
            String tokenName = tokenNameEdit.getText().toString();
            String tokenSymbol = tokenSymbolEdit.getText().toString();
            String contractAddress = tokenContractAddressEdit.getText().toString();

            addButton.setEnabled(false);
            boolean checked = false;
            for (WalletListItemModel item : walletCoinAdapter.getCheckedAssets()) {
                // We can have multiple ERC721 tokens with the same name
                if (!item.getBlockchainToken().isErc721
                        && (item.getTitle().equals(tokenName)
                                || item.getSubTitle().equals(tokenSymbol))) {
                    checked = true;
                    break;
                }
            }
            if (!contractAddress.startsWith("0x") || checked) {
                return;
            }

            AssetRatioService assetRatioService = getAssetRatioService();
            // Do not assert here, service can be null when backed from dialog
            if (assetRatioService != null) {
                assetRatioService.getTokenInfo(contractAddress, token -> {
                    if (token != null) {
                        selfChange = true;
                        tokenNameEdit.setText(token.name, TextView.BufferType.EDITABLE);
                        tokenSymbolEdit.setText(token.symbol, TextView.BufferType.EDITABLE);
                        tokenDecimalsEdit.setText(
                                String.valueOf(token.decimals), TextView.BufferType.EDITABLE);
                        if (!token.isErc721) tokenIdEdit.setEnabled(false);
                        selfChange = false;
                        addButton.setEnabled(true);
                    }
                });
            }

            if (tokenName.isEmpty() || tokenSymbol.isEmpty()
                    || tokenDecimalsEdit.getText().toString().isEmpty()) {
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
            View view, BlockchainToken[] tokens, BlockchainToken[] userSelectedTokens) {
        HashSet<String> selectedTokensSymbols = new HashSet<String>();
        for (BlockchainToken userSelectedToken : userSelectedTokens) {
            selectedTokensSymbols.add(userSelectedToken.symbol.toUpperCase(Locale.getDefault()));
        }
        RecyclerView rvAssets = view.findViewById(R.id.rvAssets);
        walletCoinAdapter = new WalletCoinAdapter(mType);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();
        for (int i = 0; i < tokens.length; i++) {
            WalletListItemModel itemModel = new WalletListItemModel(
                    R.drawable.ic_eth, tokens[i].name, tokens[i].symbol, tokens[i].tokenId, "", "");
            itemModel.setBlockchainToken(tokens[i]);
            itemModel.setIconPath("file://" + tokensPath + "/" + tokens[i].logo);

            boolean isUserSelected = selectedTokensSymbols.contains(
                    tokens[i].symbol.toUpperCase(Locale.getDefault()));
            itemModel.setIsUserSelected(isUserSelected);
            walletListItemModelList.add(itemModel);
        }
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
                    walletCoinAdapter.filter(query);
                }

                return true;
            }

            @Override
            public boolean onQueryTextChange(String newText) {
                if (walletCoinAdapter != null) {
                    walletCoinAdapter.filter(newText);
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
        Activity activity = getActivity();
        if (activity instanceof BuySendSwapActivity && checkedAssets.size() > 0) {
            BuySendSwapActivity buySendSwapActivity = (BuySendSwapActivity) activity;
            if (mType == WalletCoinAdapter.AdapterType.SEND_ASSETS_LIST
                    || mType == WalletCoinAdapter.AdapterType.BUY_ASSETS_LIST
                    || mType == WalletCoinAdapter.AdapterType.SWAP_FROM_ASSETS_LIST) {
                buySendSwapActivity.updateBuySendSwapAsset(checkedAssets.get(0).getSubTitle(),
                        checkedAssets.get(0).getBlockchainToken(), true);
                buySendSwapActivity.updateBalanceMaybeSwap(
                        buySendSwapActivity.getCurrentSelectedAccountAddr());
            } else if (mType == WalletCoinAdapter.AdapterType.SWAP_TO_ASSETS_LIST) {
                buySendSwapActivity.updateBuySendSwapAsset(checkedAssets.get(0).getSubTitle(),
                        checkedAssets.get(0).getBlockchainToken(), false);
                buySendSwapActivity.updateBalanceMaybeSwap(
                        buySendSwapActivity.getCurrentSelectedAccountAddr());
            }
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
    public void onAssetCheckedChanged(
            WalletListItemModel walletListItemModel, CheckBox assetCheck, boolean isChecked) {
        if (mType != WalletCoinAdapter.AdapterType.EDIT_VISIBLE_ASSETS_LIST) return;
        // ERC721 uses custom asset dialog
        if (walletListItemModel.getBlockchainToken().isErc721) {
            showAddAssetDialog();
            walletListItemModel.setIsUserSelected(
                    false); // The added token is different from the generic one
        } else {
            JsonRpcService jsonRpcService = getJsonRpcService();
            if (jsonRpcService == null) return;
            jsonRpcService.getNetwork(CoinType.ETH, selectedNetwork -> {
                TokenUtils.isCustomToken(getBlockchainRegistry(), selectedNetwork,
                        selectedNetwork.coin, walletListItemModel.getBlockchainToken(),
                        isCustom -> {
                            BraveWalletService braveWalletService = getBraveWalletService();
                            // TODO: all the asserts need to be removed. Shall do proper error
                            // handling instead.
                            assert braveWalletService != null;
                            if (!isCustom) {
                                if (isChecked) {
                                    braveWalletService.addUserAsset(
                                            walletListItemModel.getBlockchainToken(), (success) -> {
                                                if (success) {
                                                    walletListItemModel.setIsUserSelected(true);
                                                }
                                                if (isChecked
                                                        != walletListItemModel.getIsUserSelected())
                                                    assetCheck.setChecked(
                                                            walletListItemModel
                                                                    .getIsUserSelected());
                                            });
                                } else {
                                    braveWalletService.removeUserAsset(
                                            walletListItemModel.getBlockchainToken(), (success) -> {
                                                if (success) {
                                                    walletListItemModel.setIsUserSelected(false);
                                                }
                                                if (isChecked
                                                        != walletListItemModel.getIsUserSelected())
                                                    assetCheck.setChecked(
                                                            walletListItemModel
                                                                    .getIsUserSelected());
                                            });
                                }
                            } else {
                                braveWalletService.setUserAssetVisible(
                                        walletListItemModel.getBlockchainToken(), isChecked,
                                        success -> {
                                            if (success) {
                                                walletListItemModel.setIsUserSelected(isChecked);
                                            }
                                            if (isChecked
                                                    != walletListItemModel.getIsUserSelected())
                                                assetCheck.setChecked(
                                                        walletListItemModel.getIsUserSelected());
                                        });
                            }
                            mIsAssetsListChanged = true;
                        });
            });
        }
    };
}
