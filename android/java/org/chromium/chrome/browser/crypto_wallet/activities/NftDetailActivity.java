/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.app.Activity;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;

import com.bumptech.glide.load.DataSource;
import com.bumptech.glide.load.engine.GlideException;
import com.bumptech.glide.request.RequestListener;
import com.bumptech.glide.request.target.Target;
import com.bumptech.glide.request.transition.DrawableCrossFadeTransition;

import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.ImageLoader;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.text.NoUnderlineClickableSpan;

public class NftDetailActivity
        extends BraveWalletBaseActivity {
    private static final String TOKEN_ID_FORMAT = "#%s";
    private static final String NFT_URL_FORMAT = "%s/token/%s?a=%s";
    private String mNftName;
    private String mChainId;
    private String mContractAddress;
    private String mNftTokenId;
    private String mNftTokenHex;
    private String mNetworkName;
    private String mBlockExplorerUrl;

    private ImageView mNftImageView;
    private TextView mImageNotAvailableText;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_nft_detail);

        BraveActivity activity = BraveActivity.getBraveActivity();

        if (activity != null) {
            NetworkInfo networkInfo = activity.getWalletModel().getCryptoModel().getNetworkModel().mDefaultNetwork.getValue();
            mNetworkName = networkInfo.chainName;
            mBlockExplorerUrl = networkInfo.blockExplorerUrls.length > 0 ? networkInfo.blockExplorerUrls[0] : "";
        }

        // Calculate half screen height and assign it to NFT image view,
        // or "Image not found" text view (that will pop up in case of issues).
        int halfScreenHeight = AndroidUtils.getScreenHeight() / 2;

        mNftImageView = findViewById(R.id.nft_detail_image);
        mNftImageView.getLayoutParams().height = halfScreenHeight;

        mImageNotAvailableText = findViewById(R.id.nft_detail_image_not_available);
        mImageNotAvailableText.getLayoutParams().height = halfScreenHeight;

        if (getIntent() != null) {
            mChainId = getIntent().getStringExtra(Utils.CHAIN_ID);
            mNftName = getIntent().getStringExtra(Utils.ASSET_NAME);
            mContractAddress = getIntent().getStringExtra(Utils.ASSET_CONTRACT_ADDRESS);
            mNftTokenHex = getIntent().getStringExtra(Utils.NFT_TOKEN_ID_HEX);
            mNftTokenId = Utils.hexToIntString(mNftTokenHex);
            PortfolioModel.Erc721MetaData erc721MetaData = (PortfolioModel.Erc721MetaData) getIntent().getSerializableExtra(Utils.NFT_META_DATA);
            setMetadata(erc721MetaData);
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        Button btnSend = findViewById(R.id.btn_send);
        // TODO(simone): Enable if it's the NFT owner.
        btnSend.setVisibility(View.GONE);

        TextView nftDetailTitle = findViewById(R.id.nft_detail_title);
        nftDetailTitle.setText(Utils.formatErc721TokenTitle(mNftName, mNftTokenId));

        TextView nftName = findViewById(R.id.nft_name);
        nftName.setText(mNftName);

        TextView networkName = findViewById(R.id.blockchain_content);
        networkName.setText(mNetworkName);
        TextView tokenStandard = findViewById(R.id.token_standard_content);
        tokenStandard.setText(R.string.brave_wallet_nft_erc_721);
        TextView tokenIdView = findViewById(R.id.token_id_content);

        String tokenStr = String.format(TOKEN_ID_FORMAT, mNftTokenId);
        SpannableString spannable = new SpannableString(tokenStr);

        if (!TextUtils.isEmpty(mBlockExplorerUrl)) {
            String url = String.format(NFT_URL_FORMAT, mBlockExplorerUrl, mContractAddress, mNftTokenId);
            NoUnderlineClickableSpan linkSpan = new NoUnderlineClickableSpan(this,
                    R.color.brave_link, (textView) -> { TabUtils.openLinkWithFocus(this, url); });
            spannable.setSpan(linkSpan, 0, spannable.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        tokenIdView.setText(spannable);
        tokenIdView.setMovementMethod(LinkMovementMethod.getInstance());
        onInitialLayoutInflationComplete();
    }

    private void setMetadata(PortfolioModel.Erc721MetaData erc721MetaData) {
        // In case of no errors proceed to assign descrption and fetch NFT image.
        if (erc721MetaData.mErrCode == 0) {
            String description = erc721MetaData.mDescription;
            String imageUrl = erc721MetaData.mImageUrl;
            if (!TextUtils.isEmpty(description)) {
                TextView descriptionContent = findViewById(R.id.description_content);
                descriptionContent.setText(description);
            } else {
                // Hide description layout in case of an empty string.
                findViewById(R.id.nft_description).setVisibility(View.GONE);
            }
            if (!TextUtils.isEmpty(imageUrl)) {
                loadNftImage(imageUrl);
            } else {
                setNftImageAsNotAvailable();
            }
        }
    }

    private void loadNftImage(String imageUrl) {
        ImageLoader.getLoadNftRequest(imageUrl, this, false)
                .listener(new RequestListener<Drawable>() {
                    @Override
                    public boolean onLoadFailed(GlideException glideException, Object model, Target<Drawable> target, boolean isFirstResource) {
                        setNftImageAsNotAvailable();
                        return false;
                    }

                    @Override
                    public boolean onResourceReady(Drawable resource, Object model, Target<Drawable> target, DataSource dataSource, boolean isFirstResource) {
                        target.onResourceReady(resource, new DrawableCrossFadeTransition(250, true));
                        return true;
                    }
                })
                .into(mNftImageView);
    }

    private void setNftImageAsNotAvailable() {
        mNftImageView.setVisibility(View.GONE);
        mImageNotAvailableText.setVisibility(View.VISIBLE);
    }
}
