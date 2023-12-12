/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Context;
import android.content.Intent;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.widget.Toolbar;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSpinnerAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.util.LiveDataUtil;

import java.util.Collections;

/**
 * Activity to add new assets or NFTs depending on the boolean value passed in the intent.
 * {@see AddAssetActivity.getIntent(Context, boolean)}.
 * Used by {@code EditVisibleAssetsBottomSheetDialogFragment}.
 */
public class AddAssetActivity extends BraveWalletBaseActivity implements TextWatcher {
    private static final String TAG = "AddAssetActivity";

    private static final String NFT_CUSTOM_ASSET = "nftCustomAsset";

    private WalletModel mWalletModel;
    private NetworkModel mNetworkModel;
    private NetworkInfo mNetworkInfo;

    private boolean mNftsOnly;
    private boolean mPauseTextWatcher;
    private boolean mOnStartWithNativeCalled;

    private Toolbar mToolbar;
    private Spinner mNetworkSpinner;
    private View mTokenIdSection;
    private View mTokenDecimalSection;
    private TextView mTvDecimalTitle;
    private EditText mTokenDecimalsEdit;
    private TextView mTvAddressTitle;
    private NetworkSpinnerAdapter mNetworkAdapter;
    private Button mAdd;
    private EditText mTokenNameEdit;
    private EditText mTokenContractAddressEdit;
    private EditText mTokenSymbolEdit;
    private EditText mTokenIdEdit;

    /**
     * Gets intent to start new activity {@code AddAssetActivity}.
     * @param context Android context required to instantiate the intent.
     * @param nftOnly {@code true} if the activity will add a new NFT,
     * {@code false} to add a new custom asset.
     * @return Intent to start new activity {@code AddAssetActivity}.
     */
    public static Intent getIntent(@NonNull final Context context, final boolean nftOnly) {
        Intent intent = new Intent(context, AddAssetActivity.class);
        intent.putExtra(NFT_CUSTOM_ASSET, nftOnly);
        return intent;
    }

    @Override
    protected void onPreCreate() {
        Intent intent = getIntent();
        mNftsOnly = intent.getBooleanExtra(NFT_CUSTOM_ASSET, false);
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_add_asset);

        mToolbar = findViewById(R.id.toolbar);
        mToolbar.setTitle(mNftsOnly ? R.string.wallet_add_nft : R.string.wallet_add_custom_asset);
        setSupportActionBar(mToolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mNetworkSpinner = findViewById(R.id.network_spinner);
        mNetworkAdapter = new NetworkSpinnerAdapter(this, Collections.emptyList());
        mNetworkSpinner.setAdapter(mNetworkAdapter);
        mTokenIdSection = findViewById(R.id.token_id_section);
        mTokenDecimalSection = findViewById(R.id.token_decimal_section);
        mTvDecimalTitle = findViewById(R.id.token_decimals_title);
        mTokenDecimalsEdit = findViewById(R.id.token_decimals);
        mTvAddressTitle = findViewById(R.id.token_contract_address_title);

        mAdd = findViewById(R.id.add);
        mTokenNameEdit = findViewById(R.id.token_name);
        mTokenContractAddressEdit = findViewById(R.id.token_contract_address);
        mTokenSymbolEdit = findViewById(R.id.token_symbol);
        mTokenIdEdit = findViewById(R.id.token_id);

        onInitialLayoutInflationComplete();
    }

    @Override
    public void onStartWithNative() {
        super.onStartWithNative();
        if (mOnStartWithNativeCalled) {
            return;
        }
        mOnStartWithNativeCalled = true;

        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            mNetworkModel = mWalletModel.getNetworkModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "Error onStartWithNative.", e);
        }

        LiveDataUtil.observeOnce(mNetworkModel.mCryptoNetworks,
                networkInfoList -> { mNetworkAdapter.setNetworks(networkInfoList); });

        mNetworkSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                mNetworkInfo = mNetworkAdapter.getNetwork(position);

                mTokenDecimalSection.setVisibility(mNftsOnly ? View.GONE : View.VISIBLE);
                if (mNetworkInfo.coin == CoinType.ETH || mNetworkInfo.coin == CoinType.FIL) {
                    mTokenIdSection.setVisibility(mNftsOnly ? View.VISIBLE : View.GONE);
                    mTvAddressTitle.setText(R.string.wallet_add_custom_asset_token_address);
                } else if (mNetworkInfo.coin == CoinType.SOL) {
                    mTokenIdSection.setVisibility(View.GONE);
                    mTvAddressTitle.setText(R.string.wallet_add_custom_asset_mint_address);
                }
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) { /* No-op. */
            }
        });

        mAdd.setOnClickListener(v -> {
            BraveWalletService braveWalletService = getBraveWalletService();
            assert braveWalletService != null;
            final NetworkInfo networkInfo =
                    mNetworkAdapter.getNetwork(mNetworkSpinner.getSelectedItemPosition());
            final BlockchainToken token = new BlockchainToken();
            token.contractAddress = mTokenContractAddressEdit.getText().toString();
            token.name = mTokenNameEdit.getText().toString();
            token.logo = "";
            String tokenId = mTokenIdEdit.getText().toString();
            token.tokenId = Utils.toHex(tokenId);
            boolean isErc721 = !tokenId.trim().isEmpty();
            token.isErc20 = !isErc721;
            token.isErc721 = isErc721;
            token.isNft = isErc721;
            token.symbol = mTokenSymbolEdit.getText().toString();
            token.decimals = networkInfo.decimals;
            token.chainId = networkInfo.chainId;
            token.coin = networkInfo.coin;
            try {
                token.decimals = Integer.valueOf(mTokenDecimalsEdit.getText().toString());
            } catch (NumberFormatException numberFormatException) {
                Log.e(TAG, "Error when parsing token decimals.", numberFormatException);
            }
            if (networkInfo.coin == CoinType.SOL) {
                token.isNft = mNftsOnly;
                if (mNftsOnly) {
                    token.decimals = 0;
                }
            }
            token.visible = true;

            braveWalletService.addUserAsset(token, success -> {
                if (success) {
                    Intent intent = new Intent();
                    WalletUtils.addBlockchainTokenToIntent(intent, token);
                    WalletUtils.addNetworkInfoToIntent(intent, networkInfo);
                    setResult(RESULT_OK, intent);
                } else {
                    Log.e(TAG, "Error when adding user asset.");
                }
                finish();
            });
        });

        mTokenNameEdit.addTextChangedListener(this);
        mTokenContractAddressEdit.addTextChangedListener(this);
        mTokenSymbolEdit.addTextChangedListener(this);
        mTokenDecimalsEdit.addTextChangedListener(this);
        mTokenIdEdit.addTextChangedListener(this);
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
        if (mPauseTextWatcher) return;
        String contractAddress = mTokenContractAddressEdit.getText().toString();
        String contractAddressTrimmed = contractAddress.trim();
        if (!contractAddress.equals(contractAddressTrimmed)) {
            // Update the contractAddress and process it within the next onTextChanged pass.
            mTokenContractAddressEdit.setText(contractAddressTrimmed, TextView.BufferType.EDITABLE);
            return;
        }
        String tokenName = mTokenNameEdit.getText().toString();
        String tokenSymbol = mTokenNameEdit.getText().toString();

        mTokenIdEdit.setEnabled(true);
        mAdd.setEnabled(false);

        // AssetRatioService::GetTokenInfo has been removed in favour of
        // JsonRpcService::GetEthTokenInfo, which is a chain-agnostic method
        // to query EVM token info.
        //
        // AssetRatioService assetRatioService = getAssetRatioService();
        // assert assetRatioService != null;
        // if (!contractAddress.isEmpty() && mNetworkInfo.coin == CoinType.ETH) {
        //     assetRatioService.getTokenInfo(contractAddress, token -> {
        //         if (token != null) {
        //             mPauseTextWatcher = true;
        //             mTokenNameEdit.setText(token.name, TextView.BufferType.EDITABLE);
        //             mTokenSymbolEdit.setText(token.symbol, TextView.BufferType.EDITABLE);
        //             mTokenDecimalsEdit.setText(
        //                     String.valueOf(token.decimals), TextView.BufferType.EDITABLE);
        //             if (!token.isErc721) mTokenIdEdit.setEnabled(false);
        //             mPauseTextWatcher = false;
        //             if (!mNftsOnly
        //                     || (token.isErc721 && !mTokenIdEdit.getText().toString().isEmpty())
        //                     || !token.isErc721) {
        //                 mAdd.setEnabled(true);
        //             }
        //         }
        //     });
        // }

        if (tokenName.isEmpty()
                || tokenSymbol.isEmpty()
                || (mNetworkInfo.coin == CoinType.ETH
                        && ((mNftsOnly && mTokenIdEdit.getText().toString().isEmpty())
                                || mTokenDecimalsEdit.getText().toString().isEmpty()))) {
            return;
        }

        mAdd.setEnabled(true);
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) { /* No-op. */
    }

    @Override
    public void afterTextChanged(Editable s) { /* No-op. */
    }
}
