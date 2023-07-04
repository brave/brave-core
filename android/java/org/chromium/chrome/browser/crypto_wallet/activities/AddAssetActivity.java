/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Intent;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;

public class AddAssetActivity extends BraveWalletBaseActivity {
    private static final String TAG = "AddAssetActivity";

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_add_asset);

        Intent intent = getIntent();
//        if (intent != null) {
//            //mChainId = intent.getStringExtra(CHAIN_ID);
//        }


        BraveActivity braveActivity = null;
        try {
            braveActivity = BraveActivity.getBraveActivity();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "triggerLayoutInflation " + e);
        }
        assert braveActivity != null;
    }

//    public static Intent getIntent(Context context, String chainId, BlockchainToken asset,
//            PortfolioModel.NftDataModel nftDataModel) {
//        Intent intent = new Intent(context, NftDetailActivity.class);
//        intent.putExtra(CHAIN_ID, chainId);
//        intent.putExtra(ASSET_NAME, asset.name);
//        intent.putExtra(ASSET_CONTRACT_ADDRESS, asset.contractAddress);
//        intent.putExtra(ASSET_SYMBOL, asset.symbol);
//        intent.putExtra(NFT_TOKEN_ID_HEX, asset.tokenId);
//        intent.putExtra(NFT_META_DATA, nftDataModel.nftMetadata);
//        intent.putExtra(NFT_IS_ERC_721, nftDataModel.token.isErc721);
//        intent.putExtra(COIN_TYPE, asset.coin);
//        return intent;
//    }
}
