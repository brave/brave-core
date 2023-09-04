/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;

import com.bumptech.glide.Glide;
import com.bumptech.glide.request.target.Target;
import com.bumptech.glide.request.transition.DrawableCrossFadeTransition;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.app.helpers.ImageLoader;
import org.chromium.chrome.browser.crypto_wallet.util.AddressUtils;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.text.NoUnderlineClickableSpan;

import java.util.Locale;

public class NftDetailActivity extends BraveWalletBaseActivity {
    private static final String TAG = "NftDetailActivity";

    private static final String TOKEN_ID_FORMAT = "#%s";
    private static final String NFT_ERC721_URL_FORMAT = "%s/token/%s?a=%s";
    private static final String NFT_SPL_URL_FORMAT = "%s/address/%s";
    private static final String NFT_SPL_URL_FORMAT_WITH_CLUSTER =
            NFT_SPL_URL_FORMAT + "/?cluster=%s";

    private static final String CHAIN_ID = "chainId";
    private static final String ASSET_NAME = "assetName";
    private static final String ASSET_CONTRACT_ADDRESS = "assetContractAddress";
    private static final String ASSET_SYMBOL = "assetSymbol";
    private static final String NFT_TOKEN_ID_HEX = "nftTokenIdHex";
    private static final String NFT_META_DATA = "nftMetadata";
    private static final String NFT_IS_ERC_721 = "nftIsErc721";
    private static final String COIN_TYPE = "coinType";

    private String mNftName;
    private String mChainId;
    private String mContractAddress;
    private String mSymbol;
    private String mNftTokenId;
    private String mNftTokenHex;

    private boolean mIsErc721;

    private int mCoinType;

    private ImageView mNftImageView;
    private TextView mImageNotAvailableText;
    private TextView mNetworkNameView;
    private Button mBtnSend;
    private TextView mTokenStandardView;
    private TextView mTokenAddressLabelView;
    private TextView mTokenAddressView;
    private TextView mDescriptionContentView;
    private TextView mNftDetailTitleView;
    private TextView mNftNameView;
    private ViewGroup mNftDescriptionLayout;
    private ViewGroup mNftTokenStandardLayout;
    private ViewGroup mNftTokenAddressLayout;
    private Toolbar mToolbar;

    private PortfolioModel.NftMetadata mNftMetadata;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_nft_detail);

        Intent intent = getIntent();
        if (intent != null) {
            mChainId = intent.getStringExtra(CHAIN_ID);
            mNftName = intent.getStringExtra(ASSET_NAME);
            mContractAddress = intent.getStringExtra(ASSET_CONTRACT_ADDRESS);
            mSymbol = intent.getStringExtra(ASSET_SYMBOL);
            mNftTokenHex = intent.getStringExtra(NFT_TOKEN_ID_HEX);
            mNftTokenId = Utils.hexToIntString(mNftTokenHex);
            mNftMetadata = (PortfolioModel.NftMetadata) intent.getSerializableExtra(NFT_META_DATA);
            mIsErc721 = intent.getBooleanExtra(NFT_IS_ERC_721, false);
            mCoinType = intent.getIntExtra(COIN_TYPE, -1);
        }

        // Calculate half screen height and assign it to NFT image view,
        // and "Image not found" text view (that will pop up in case of issues).
        int halfScreenHeight = AndroidUtils.getScreenHeight() / 2;

        mNftImageView = findViewById(R.id.nft_detail_image);
        mNftImageView.getLayoutParams().height = halfScreenHeight;

        mImageNotAvailableText = findViewById(R.id.nft_detail_image_not_available);
        mImageNotAvailableText.getLayoutParams().height = halfScreenHeight;

        mDescriptionContentView = findViewById(R.id.description_content);

        mToolbar = findViewById(R.id.toolbar);
        setSupportActionBar(mToolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mBtnSend = findViewById(R.id.btn_send);
        // TODO(simone): Enable if it's the NFT owner.
        // GitHub issue: https://github.com/brave/brave-browser/issues/27802.
        mBtnSend.setVisibility(View.GONE);

        mNftDetailTitleView = findViewById(R.id.nft_detail_title);
        mNftDetailTitleView.setText(Utils.formatErc721TokenTitle(mNftName, mNftTokenId));

        mNftNameView = findViewById(R.id.nft_name);
        mNftNameView.setText(mSymbol);

        mNetworkNameView = findViewById(R.id.blockchain_content);
        mNftTokenStandardLayout = findViewById(R.id.nft_token_standard);
        mNftTokenAddressLayout = findViewById(R.id.nft_token_address);
        mTokenStandardView = findViewById(R.id.token_standard_content);
        mTokenAddressLabelView = findViewById(R.id.token_address_label);

        if (mIsErc721) {
            mTokenStandardView.setText(R.string.brave_wallet_nft_erc_721);
            mTokenAddressLabelView.setText(R.string.brave_wallet_nft_token_id);
        } else if (mCoinType == CoinType.SOL) {
            mTokenStandardView.setText(R.string.brave_wallet_nft_sol_spl);
            mTokenAddressLabelView.setText(R.string.brave_wallet_nft_mint_address);
        } else {
            // Not ERC 721, nor Solana NFT.
            // Hiding incompatible lables.
            AndroidUtils.gone(mNftTokenStandardLayout, mNftTokenAddressLayout);
        }

        mTokenAddressView = findViewById(R.id.token_address_content);

        mNftDescriptionLayout = findViewById(R.id.nft_description);

        setMetadata(mNftMetadata);
        BraveActivity braveActivity = null;
        try {
            braveActivity = BraveActivity.getBraveActivity();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "triggerLayoutInflation " + e);
        }
        assert braveActivity != null;

        LiveDataUtil.observeOnce(
                braveActivity.getWalletModel().getCryptoModel().getNetworkModel().mDefaultNetwork,
                defaultNetwork -> {
                    String networkName = defaultNetwork.chainName;
                    mNetworkNameView.setText(networkName);
                    String blockExplorerUrl = defaultNetwork.blockExplorerUrls.length > 0
                            ? defaultNetwork.blockExplorerUrls[0]
                            : "";

                    SpannableString spannable;
                    if (mIsErc721) {
                        String tokenStr =
                                String.format(Locale.ENGLISH, TOKEN_ID_FORMAT, mNftTokenId);
                        spannable = new SpannableString(tokenStr);

                        if (!TextUtils.isEmpty(blockExplorerUrl)) {
                            String url = String.format(Locale.ENGLISH, NFT_ERC721_URL_FORMAT,
                                    blockExplorerUrl, mContractAddress, mNftTokenId);
                            createClickableLink(blockExplorerUrl, url, spannable);
                        }

                    } else {
                        String mintAddress = AddressUtils.getTruncatedAddress(mContractAddress);
                        spannable = new SpannableString(mintAddress);

                        if (!TextUtils.isEmpty(blockExplorerUrl)) {
                            // Blockchain explorer URLs may contain a cluster endpoint.
                            // When present it must be appended at the end of the formatted URL.
                            String[] splitBlockExplorerUrl = blockExplorerUrl.split("/\\?cluster=");
                            String baseUrl = splitBlockExplorerUrl[0];
                            String url;
                            if (splitBlockExplorerUrl.length > 1) {
                                String cluster = splitBlockExplorerUrl[1];
                                url = String.format(Locale.ENGLISH, NFT_SPL_URL_FORMAT_WITH_CLUSTER,
                                        baseUrl, mContractAddress, cluster);
                            } else {
                                url = String.format(Locale.ENGLISH, NFT_SPL_URL_FORMAT, baseUrl,
                                        mContractAddress);
                            }
                            createClickableLink(blockExplorerUrl, url, spannable);
                        }
                    }

                    mTokenAddressView.setText(spannable);
                    mTokenAddressView.setMovementMethod(LinkMovementMethod.getInstance());

                    onInitialLayoutInflationComplete();
                });
    }

    private void createClickableLink(
            String blockExplorerUrl, String url, SpannableString spannable) {
        NoUnderlineClickableSpan linkSpan = new NoUnderlineClickableSpan(
                this, R.color.brave_link, (textView) -> { TabUtils.openLinkWithFocus(this, url); });
        spannable.setSpan(linkSpan, 0, spannable.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    }

    private void setMetadata(PortfolioModel.NftMetadata nftMetadata) {
        if (nftMetadata == null) return;
        // In case of no errors proceed to assign description and fetch NFT image.
        if (nftMetadata.mErrCode == 0) {
            String description = nftMetadata.mDescription;
            String imageUrl = nftMetadata.mImageUrl;
            if (!TextUtils.isEmpty(description)) {
                mDescriptionContentView.setText(description);
            } else {
                // Hide description layout in case of an empty string.
                mNftDescriptionLayout.setVisibility(View.GONE);
            }
            if (ImageLoader.isSupported(imageUrl)) {
                loadNftImage(imageUrl);
            } else {
                setNftImageAsNotAvailable();
            }
        }
    }

    private void loadNftImage(String imageUrl) {
        ImageLoader.downloadImage(imageUrl, Glide.with(this), false,
                WalletConstants.RECT_ROUNDED_CORNERS_DP, mNftImageView, new ImageLoader.Callback() {
                    @Override
                    public boolean onLoadFailed() {
                        setNftImageAsNotAvailable();
                        return false;
                    }
                    @Override
                    public boolean onResourceReady(Drawable resource, Target<Drawable> target) {
                        target.onResourceReady(
                                resource, new DrawableCrossFadeTransition(250, true));
                        return true;
                    }
                });
    }

    private void setNftImageAsNotAvailable() {
        mNftImageView.setVisibility(View.GONE);
        mImageNotAvailableText.setVisibility(View.VISIBLE);
    }

    public static Intent getIntent(Context context, String chainId, BlockchainToken asset,
            PortfolioModel.NftDataModel nftDataModel) {
        Intent intent = new Intent(context, NftDetailActivity.class);
        intent.putExtra(CHAIN_ID, chainId);
        intent.putExtra(ASSET_NAME, asset.name);
        intent.putExtra(ASSET_CONTRACT_ADDRESS, asset.contractAddress);
        intent.putExtra(ASSET_SYMBOL, asset.symbol);
        intent.putExtra(NFT_TOKEN_ID_HEX, asset.tokenId);
        intent.putExtra(NFT_META_DATA, nftDataModel.nftMetadata);
        intent.putExtra(NFT_IS_ERC_721, nftDataModel.token.isErc721);
        intent.putExtra(COIN_TYPE, asset.coin);
        return intent;
    }
}
