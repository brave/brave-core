/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.StringRes;
import androidx.recyclerview.widget.RecyclerView;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.OriginInfo;
import org.chromium.brave_wallet.mojom.SignDataUnion;
import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.brave_wallet.mojom.SiweMessage;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.DappsModel;
import org.chromium.chrome.browser.app.helpers.ImageLoader;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.TwoLineItemBottomSheetFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.WalletBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.url.mojom.Url;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

/** Fragment used by DApps sign in operation */
@NullMarked
public class SiweMessageFragment extends WalletBottomSheetDialogFragment {
    private static final String TAG = "SignMessageFragment";

    @Nullable private SignMessageRequest mCurrentSignMessageRequest;
    @Nullable private SiweMessage mSiweMessageData;
    private TextView mTvOrigin;
    private TextView mTvUrl;
    private ImageView mIvFav;
    private ImageView mIvFavNetwork;
    private TextView mNetworkName;
    private Button mBtCancel;
    protected Button mBtSign;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private View mIvFavNetworkContainer;
    private RecyclerView mRvDetails;
    private TwoLineItemRecyclerViewAdapter mTwoLineAdapter;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_siwe, container, false);
        mIvFav = view.findViewById(R.id.frag_siwe_iv_fav);
        mTvOrigin = view.findViewById(R.id.frag_siwe_tv_origin);
        mTvUrl = view.findViewById(R.id.frag_siwe_tv_url);
        mIvFavNetwork = view.findViewById(R.id.frag_siwe_iv_fav_nw_icon);
        mNetworkName = view.findViewById(R.id.frag_siwe_msg_tv_network_name);
        mIvFavNetworkContainer = view.findViewById(R.id.frag_siwe_cv_fav_nw_container);
        mRvDetails = view.findViewById(R.id.frag_siwe_rv);
        mTwoLineAdapter = new TwoLineItemRecyclerViewAdapter(Collections.emptyList());
        mRvDetails.setAdapter(mTwoLineAdapter);

        mBtCancel = view.findViewById(R.id.frag_siwe_msg_btn_cancel);
        mBtSign = view.findViewById(R.id.frag_siwe_msg_btn_sign);
        fillSignMessageInfo(true);

        mBtSign.setText(R.string.brave_wallet_sign_in_message_positive_button_action);
        return view;
    }

    private void notifySignMessageRequestProcessed(boolean isApproved) {
        if (mCurrentSignMessageRequest == null) return;
        getWalletModel()
                .getDappsModel()
                .notifySignMessageRequestProcessed(isApproved, mCurrentSignMessageRequest.id);
        // Nothing on screen is actionable until the next request is bound.
        mCurrentSignMessageRequest = null;
        mSiweMessageData = null;
        fillSignMessageInfo(false);
    }

    private void fillSignMessageInfo(boolean init) {
        getWalletModel()
                .getDappsModel()
                .getPendingSignMessageRequests(
                        requests -> {
                            if (!isAdded()) {
                                return;
                            }
                            if (requests == null || requests.length == 0) {
                                getWalletFragmentCallback().onAllRequestsHandled();
                                return;
                            }

                            final SignMessageRequest request = requests[0];
                            final BraveWalletDAppsActivity.ActivityType panel =
                                    DappsModel.getSignMessagePanel(request);
                            if (panel != BraveWalletDAppsActivity.ActivityType.SIWE_MESSAGE) {
                                // Only sign-in requests are shown here, the generic sign panel
                                // shows the rest.
                                getWalletFragmentCallback().showPanel(panel);
                                return;
                            }
                            mCurrentSignMessageRequest = request;
                            mSiweMessageData = request.signData.getEthSiweData();
                            if (init) {
                                mBtCancel.setOnClickListener(
                                        v -> notifySignMessageRequestProcessed(false));
                                mBtSign.setOnClickListener(
                                        v -> notifySignMessageRequestProcessed(true));
                            }
                            if (request.originInfo != null
                                    && URLUtil.isValidUrl(request.originInfo.originSpec)) {
                                mTvOrigin.setText(request.originInfo.eTldPlusOne);
                                mTvUrl.setText(Utils.geteTldSpanned(request.originInfo));
                            } else {
                                // Remove the previous request's origin.
                                mTvOrigin.setText("");
                                mTvUrl.setText("");
                            }
                            updateDetails(request.chainId, request.accountId);
                            updateNetwork(request.chainId);
                            updateFavIcon(request);
                        });
    }

    private void updateDetails(String chainId, AccountId accountId) {
        // Remove the previous request's details, in case this one cannot be described.
        setDetails(Collections.emptyList());
        if (TextUtils.isEmpty(chainId)) {
            return;
        }
        NetworkInfo network = getWalletModel().getNetworkModel().getNetwork(chainId);
        if (network == null || accountId == null) return;
        assert (accountId.coin == CoinType.ETH);
        AccountInfo accountInfo = getKeyringModel().getAccount(accountId.address);
        if (accountInfo == null) return;
        assert (accountInfo.address != null);

        List<TwoLineItemRecyclerViewAdapter.TwoLineItem> items = new ArrayList<>();
        TwoLineItemRecyclerViewAdapter.TwoLineItemText account =
                new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                        accountInfo.name, Utils.getTruncatedAddress(accountInfo.address));
        account.imageType = TwoLineItemRecyclerViewAdapter.ImageType.BLOCKIE;
        account.imgData = accountId.address;
        items.add(account);

        if (mCurrentSignMessageRequest != null && mCurrentSignMessageRequest.originInfo != null) {
            TwoLineItemRecyclerViewAdapter.TwoLineSingleText details =
                    new TwoLineItemRecyclerViewAdapter.TwoLineSingleText();
            String separator = System.getProperty(WalletConstants.LINE_SEPARATOR);
            SpannableStringBuilder builder = new SpannableStringBuilder();
            builder.append(separator);
            builder.append(
                    getString(
                            R.string.brave_wallet_sign_in_with_brave_wallet_message,
                            mCurrentSignMessageRequest.originInfo.eTldPlusOne));
            builder.append(separator);
            builder.append(
                    AndroidUtils.createClickableSpanString(
                            requireContext(),
                            R.string.brave_wallet_see_details,
                            (view) -> showSiweDetails()));
            details.updateViewCb =
                    textView -> {
                        textView.setMovementMethod(LinkMovementMethod.getInstance());
                        textView.setText(builder);
                    };
            items.add(details);
            items.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemDivider());
        }
        if (mSiweMessageData != null) {
            if (!TextUtils.isEmpty(mSiweMessageData.statement)) {
                TwoLineItemRecyclerViewAdapter.TwoLineItemText message =
                        new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                                getString(R.string.message) + ":", mSiweMessageData.statement);
                items.add(message);
            }
            if (mSiweMessageData.resources != null && mSiweMessageData.resources.length > 0) {
                TwoLineItemRecyclerViewAdapter.TwoLineItemText resource =
                        new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                                getString(R.string.resources) + ":",
                                getSiweResources(mSiweMessageData.resources));
                items.add(resource);
            }
        }
        mTwoLineAdapter.mDividerMargin = 12;
        setDetails(items);
    }

    /** Replaces the whole detail list, which has no stable item identity to diff against. */
    private void setDetails(final List<TwoLineItemRecyclerViewAdapter.TwoLineItem> items) {
        final int previousCount = mTwoLineAdapter.getItemCount();
        mTwoLineAdapter.setValues(items);
        mTwoLineAdapter.notifyItemRangeRemoved(0, previousCount);
        mTwoLineAdapter.notifyItemRangeInserted(0, items.size());
    }

    @SuppressWarnings("NoStreams")
    private String getSiweResources(Url @Nullable [] urls) {
        if (urls == null || urls.length == 0) return "";
        return Arrays.stream(urls)
                .filter(Objects::nonNull)
                .map(url -> url.url)
                .collect(Collectors.joining(System.getProperty(WalletConstants.LINE_SEPARATOR)));
    }

    private void updateFavIcon(final SignMessageRequest request) {
        // Remove the previous request's icon while this one loads.
        mIvFav.setImageDrawable(null);
        final OriginInfo originInfo = request.originInfo;
        if (originInfo == null || !URLUtil.isValidUrl(originInfo.originSpec)) {
            setAccountBlockies(request.accountId.address);
            return;
        }
        ImageLoader.fetchFavIcon(
                originInfo.originSpec,
                false,
                new WeakReference<>(getContext()),
                bitmap -> {
                    // The icon can arrive after the panel has moved on.
                    if (request != mCurrentSignMessageRequest) {
                        return;
                    }
                    if (bitmap != null) {
                        mIvFav.setImageBitmap(bitmap);
                    } else {
                        setAccountBlockies(request.accountId.address);
                    }
                });
    }

    private void setAccountBlockies(@Nullable final String accountAddress) {
        if (accountAddress != null) {
            Utils.setTextGeneratedBlockies(mExecutor, mHandler, mIvFav, accountAddress, true);
        }
    }

    private void updateNetwork(String chainId) {
        final NetworkInfo network =
                TextUtils.isEmpty(chainId)
                        ? null
                        : getWalletModel().getNetworkModel().getNetwork(chainId);
        mNetworkName.setText(network != null ? network.chainName : "");
        final String logo = network != null ? Utils.getNetworkIconName(network) : null;
        if (TextUtils.isEmpty(logo)) {
            // Remove the previous request's network icon.
            mIvFavNetworkContainer.setVisibility(View.GONE);
            return;
        }
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();
        Utils.setBitmapResource(
                mExecutor,
                mHandler,
                requireContext(),
                "file://" + tokensPath + "/" + logo,
                Integer.MIN_VALUE,
                mIvFavNetwork,
                null,
                true);
        mIvFavNetworkContainer.setVisibility(View.VISIBLE);
    }

    private void showSiweDetails() {
        if (mCurrentSignMessageRequest == null || mSiweMessageData == null) {
            return;
        }
        assert mCurrentSignMessageRequest.signData.which() == SignDataUnion.Tag.EthSiweData;

        List<TwoLineItemRecyclerViewAdapter.TwoLineItem> items = new ArrayList<>();
        addDetail(items, R.string.brave_wallet_origin, getOriginJson(mSiweMessageData.origin));
        addDetail(items, R.string.brave_wallet_address, mSiweMessageData.address);
        addDetail(items, R.string.brave_wallet_statement, mSiweMessageData.statement);
        addDetail(items, R.string.brave_wallet_uri, mSiweMessageData.uri.url);
        addDetail(items, R.string.brave_wallet_version, Integer.toString(mSiweMessageData.version));
        addDetail(items, R.string.brave_wallet_chain_id, Long.toString(mSiweMessageData.chainId));
        addDetail(items, R.string.brave_wallet_nonce, mSiweMessageData.nonce);
        addDetail(items, R.string.brave_wallet_issued_at, mSiweMessageData.issuedAt);
        addDetail(items, R.string.brave_wallet_expiration_time, mSiweMessageData.expirationTime);
        addDetail(items, R.string.brave_wallet_not_before, mSiweMessageData.notBefore);
        addDetail(items, R.string.brave_wallet_request_id, mSiweMessageData.requestId);
        addDetail(items, R.string.resources, getSiweResources(mSiweMessageData.resources));
        TwoLineItemBottomSheetFragment fragment = new TwoLineItemBottomSheetFragment();
        fragment.setItems(items);
        fragment.show(getParentFragmentManager(), TAG);
    }

    private void addDetail(
            List<TwoLineItemRecyclerViewAdapter.TwoLineItem> allDetails,
            @StringRes int id,
            @Nullable String value) {
        if (TextUtils.isEmpty(value)) return;
        allDetails.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemText(getString(id), value));
        allDetails.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemDivider());
    }

    @Nullable
    private String getOriginJson(org.chromium.url.internal.mojom.Origin origin) {
        try {
            JSONObject jsonObject = new JSONObject();

            jsonObject.put("scheme", origin.scheme);
            jsonObject.put("host", origin.host);
            jsonObject.put("port", origin.port);

            return jsonObject.toString();
        } catch (JSONException e) {
            Log.e(TAG, "Error when extracting Siwe message data origin.", e);
        }

        return null;
    }
}
