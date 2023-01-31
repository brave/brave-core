/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.app.Activity;
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

import com.bumptech.glide.load.DataSource;
import com.bumptech.glide.load.engine.GlideException;
import com.bumptech.glide.request.RequestListener;
import com.bumptech.glide.request.target.Target;
import com.bumptech.glide.request.transition.DrawableCrossFadeTransition;

import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.ImageLoader;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.text.NoUnderlineClickableSpan;

public class NftDetailActivity extends BraveWalletBaseActivity {
    private static final String TOKEN_ID_FORMAT = "#%s";
    private static final String NFT_URL_FORMAT = "%s/token/%s?a=%s";

    private static final String CHAIN_ID = "chainId";
    private static final String ASSET_NAME = "assetName";
    private static final String ASSET_CONTRACT_ADDRESS = "assetContractAddress";
    private static final String NFT_TOKEN_ID_HEX = "nftTokenIdHex";
    private static final String NFT_META_DATA = "nftMetaData";

    private String mNftName;
    private String mChainId;
    private String mContractAddress;
    private String mNftTokenId;
    private String mNftTokenHex;

    private ImageView mNftImageView;
    private TextView mImageNotAvailableText;
    private TextView mNetworkNameView;
    private Button mBtnSend;
    private TextView mTokenStandardView;
    private TextView mTokenIdView;
    private TextView mDescriptionContentView;
    private TextView mNftDetailTitleView;
    private TextView mNftNameView;
    private ViewGroup mNftDescriptionLayout;
    private Toolbar mToolbar;

    private PortfolioModel.Erc721MetaData mErc721MetaData;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_nft_detail);

        Intent intent = getIntent();
        if (intent != null) {
            mChainId = intent.getStringExtra(CHAIN_ID);
            mNftName = intent.getStringExtra(ASSET_NAME);
            mContractAddress = intent.getStringExtra(ASSET_CONTRACT_ADDRESS);
            mNftTokenHex = intent.getStringExtra(NFT_TOKEN_ID_HEX);
            mNftTokenId = Utils.hexToIntString(mNftTokenHex);
            mErc721MetaData =
                    (PortfolioModel.Erc721MetaData) intent.getSerializableExtra(NFT_META_DATA);
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
        mBtnSend.setVisibility(View.GONE);

        mNftDetailTitleView = findViewById(R.id.nft_detail_title);
        mNftDetailTitleView.setText(Utils.formatErc721TokenTitle(mNftName, mNftTokenId));

        mNftNameView = findViewById(R.id.nft_name);
        mNftNameView.setText(mNftName);

        mNetworkNameView = findViewById(R.id.blockchain_content);

        mTokenStandardView = findViewById(R.id.token_standard_content);
        mTokenStandardView.setText(R.string.brave_wallet_nft_erc_721);

        mTokenIdView = findViewById(R.id.token_id_content);

        mNftDescriptionLayout = findViewById(R.id.nft_description);

        setMetadata(mErc721MetaData);

        BraveActivity braveActivity = BraveActivity.getBraveActivity();
        assert braveActivity != null;

        LiveDataUtil.observeOnce(
                braveActivity.getWalletModel().getCryptoModel().getNetworkModel().mDefaultNetwork,
                defaultNetwork -> {
                    String networkName = defaultNetwork.chainName;
                    mNetworkNameView.setText(networkName);
                    String blockExplorerUrl = defaultNetwork.blockExplorerUrls.length > 0
                            ? defaultNetwork.blockExplorerUrls[0]
                            : "";

                    String tokenStr = String.format(TOKEN_ID_FORMAT, mNftTokenId);
                    SpannableString spannable = new SpannableString(tokenStr);

                    if (!TextUtils.isEmpty(blockExplorerUrl)) {
                        String url = String.format(
                                NFT_URL_FORMAT, blockExplorerUrl, mContractAddress, mNftTokenId);
                        NoUnderlineClickableSpan linkSpan =
                                new NoUnderlineClickableSpan(this, R.color.brave_link,
                                        (textView) -> { TabUtils.openLinkWithFocus(this, url); });
                        spannable.setSpan(
                                linkSpan, 0, spannable.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                    }
                    mTokenIdView.setText(spannable);
                    mTokenIdView.setMovementMethod(LinkMovementMethod.getInstance());

                    onInitialLayoutInflationComplete();
                });
    }

    private void setMetadata(PortfolioModel.Erc721MetaData erc721MetaData) {
        if (erc721MetaData == null) return;
        // In case of no errors proceed to assign description and fetch NFT image.
        if (erc721MetaData.mErrCode == 0) {
            String description = erc721MetaData.mDescription;
            String imageUrl = erc721MetaData.mImageUrl;
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
        ImageLoader.createLoadNftRequest(imageUrl, this, false)
                .listener(new RequestListener<Drawable>() {
                    @Override
                    public boolean onLoadFailed(GlideException glideException, Object model,
                            Target<Drawable> target, boolean isFirstResource) {
                        setNftImageAsNotAvailable();
                        return false;
                    }

                    @Override
                    public boolean onResourceReady(Drawable resource, Object model,
                            Target<Drawable> target, DataSource dataSource,
                            boolean isFirstResource) {
                        target.onResourceReady(
                                resource, new DrawableCrossFadeTransition(250, true));
                        return true;
                    }
                })
                .into(mNftImageView);
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
        intent.putExtra(NFT_TOKEN_ID_HEX, asset.tokenId);
        intent.putExtra(NFT_META_DATA, nftDataModel.erc721MetaData);
        return intent;
    }
}
