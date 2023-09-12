/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.SpannableString;
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

import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.OriginInfo;
import org.chromium.brave_wallet.mojom.SignDataUnion;
import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.brave_wallet.mojom.SiweMessage;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.app.helpers.ImageLoader;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.WalletBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.util.AddressUtils;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.url.mojom.Url;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

/**
 * Fragment used by DApps sign in operation
 */
public class SiweMessageFragment extends WalletBottomSheetDialogFragment {
    private static final String TAG = "SignMessageFragment";

    private List<String> mTabTitles;
    private SignMessageRequest mCurrentSignMessageRequest;
    private SiweMessage mSiweMessage;
    private ImageView mIvFav;
    private ImageView mIvFavNetwork;
    private TextView mNetworkName;
    private Button mBtCancel;
    protected Button mBtSign;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private WalletModel mWalletModel;
    private TextView mTvAccountDetails;
    private View mIvFavNetworkContainer;
    private RecyclerView mRvDetails;
    private TwoLineItemRecyclerViewAdapter mTwoLineAdapter;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mTabTitles = new ArrayList<>();
        mTabTitles.add(getString(R.string.details));
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            registerKeyringObserver(mWalletModel.getKeyringModel());
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate ", e);
        }
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_siwe, container, false);
        mIvFav = view.findViewById(R.id.frag_siwe_iv_fav);
        mIvFavNetwork = view.findViewById(R.id.frag_siwe_iv_fav_nw_icon);
        mIvFavNetworkContainer = view.findViewById(R.id.frag_siwe_cv_fav_nw_container);
        mTvAccountDetails = view.findViewById(R.id.frag_siwe_tv_account);
        mNetworkName = view.findViewById(R.id.frag_siwe_msg_tv_network_name);
        mRvDetails = view.findViewById(R.id.frag_siwe_rv);
        mTwoLineAdapter = new TwoLineItemRecyclerViewAdapter(Collections.emptyList());
        mRvDetails.setAdapter(mTwoLineAdapter);

        mBtCancel = view.findViewById(R.id.frag_siwe_msg_btn_cancel);
        mBtSign = view.findViewById(R.id.frag_siwe_msg_btn_sign);
        initComponents();

        mBtSign.setText(R.string.brave_wallet_sign_in_message_positive_button_action);
        return view;
    }

    private void notifySignMessageRequestProcessed(boolean approved) {
        getBraveWalletService().notifySignMessageRequestProcessed(
                approved, mCurrentSignMessageRequest.id, null, null);
        fillSignMessageInfo(false);
    }

    private void initComponents() {
        fillSignMessageInfo(true);
    }

    private void fillSignMessageInfo(boolean init) {
        getBraveWalletService().getPendingSignMessageRequests(requests -> {
            if (requests == null || requests.length == 0) {
                Intent intent = new Intent();
                getActivity().setResult(Activity.RESULT_OK, intent);
                getActivity().finish();
                return;
            }

            mCurrentSignMessageRequest = requests[0];
            if (mCurrentSignMessageRequest.signData.which() == SignDataUnion.Tag.EthSiweData) {
                mSiweMessage = mCurrentSignMessageRequest.signData.getEthSiweData();
            }
            if (init) {
                mBtCancel.setOnClickListener(v -> { notifySignMessageRequestProcessed(false); });
                mBtSign.setOnClickListener(v -> { notifySignMessageRequestProcessed(true); });
            }
            updateDetails(mCurrentSignMessageRequest.chainId, mCurrentSignMessageRequest.accountId);
            updateNetwork(mCurrentSignMessageRequest.chainId);
            updateFavIcon(mCurrentSignMessageRequest.originInfo,
                    mCurrentSignMessageRequest.accountId.address);
        });
    }

    private void updateDetails(String chainId, AccountId accountId) {
        if (JavaUtils.anyNull(mWalletModel, chainId)) return;
        NetworkInfo network = mWalletModel.getNetworkModel().getNetwork(chainId);
        if (network == null || accountId == null) return;
        assert (accountId.coin == CoinType.ETH);
        AccountInfo accountInfo = mWalletModel.getKeyringModel().getAccount(accountId.address);
        if (accountInfo == null) return;
        assert (accountInfo.address != null);

        String accountText =
                accountInfo.name + "\n" + AddressUtils.getTruncatedAddress(accountInfo.address);
        mTvAccountDetails.setText(accountText);

        List<TwoLineItemRecyclerViewAdapter.TwoLineItem> items = new ArrayList<>();
        TwoLineItemRecyclerViewAdapter.TwoLineItemText account =
                new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                        accountInfo.name, AddressUtils.getTruncatedAddress(accountInfo.address));
        account.imageType = TwoLineItemRecyclerViewAdapter.ImageType.BLOCKIE;
        account.imgData = accountId.address;
        items.add(account);

        if (mCurrentSignMessageRequest.originInfo != null) {
            TwoLineItemRecyclerViewAdapter.TwoLineSingleText details =
                    new TwoLineItemRecyclerViewAdapter.TwoLineSingleText();
            String separator = System.getProperty(WalletConstants.LINE_SEPARATOR);
            SpannableStringBuilder builder = new SpannableStringBuilder();
            builder.append(separator);
            builder.append(getString(R.string.brave_wallet_sign_in_with_brave_wallet_message,
                    mCurrentSignMessageRequest.originInfo.eTldPlusOne));
            SpannableString seeDetails = Utils.createSpanForSurroundedPhrase(
                    requireContext(), R.string.brave_wallet_see_details, (v) -> {});
            builder.append(separator);
            builder.append(seeDetails);
            details.updateViewCb = textView -> {
                textView.setMovementMethod(LinkMovementMethod.getInstance());
                textView.setText(builder);
            };
            items.add(details);
            items.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemDivider());
        }
        if (mSiweMessage != null) {
            if (!TextUtils.isEmpty(mSiweMessage.statement)) {
                TwoLineItemRecyclerViewAdapter.TwoLineItemText message =
                        new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                                getString(R.string.message) + ":", mSiweMessage.statement);
                items.add(message);
            }
            if (mSiweMessage.resources != null && mSiweMessage.resources.length > 0) {
                TwoLineItemRecyclerViewAdapter.TwoLineItemText resource =
                        new TwoLineItemRecyclerViewAdapter.TwoLineItemText(
                                getString(R.string.resources) + ":",
                                getSiweResources(mSiweMessage.resources));
                items.add(resource);
            }
        }
        mTwoLineAdapter.setValues(items);
        mTwoLineAdapter.notifyItemRangeChanged(0, items.size());
    }

    private String getSiweResources(Url[] urls) {
        if (urls == null || urls.length == 0) return "";
        return Arrays.stream(urls)
                .filter(url -> url != null)
                .map(url -> url.url)
                .collect(Collectors.joining(System.getProperty(WalletConstants.LINE_SEPARATOR)));
    }

    private void updateFavIcon(OriginInfo originInfo, String accountAddress) {
        if (originInfo == null || !URLUtil.isValidUrl(originInfo.originSpec)) return;
        ImageLoader.fetchFavIcon(
                originInfo.originSpec, new WeakReference<>(getContext()), bitmap -> {
                    if (bitmap != null) {
                        mIvFav.setImageBitmap(bitmap);
                    } else if (accountAddress != null) {
                        Utils.setTextGeneratedBlockies(
                                mExecutor, mHandler, mIvFav, accountAddress, true);
                    }
                });
    }

    private void updateNetwork(String chainId) {
        if (JavaUtils.anyNull(mWalletModel, chainId)) return;
        NetworkInfo network = mWalletModel.getNetworkModel().getNetwork(chainId);
        if (network == null) return;
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();
        String logo = Utils.getNetworkIconName(network.chainId, network.coin);
        if (TextUtils.isEmpty(logo)) return;
        Utils.setBitmapResource(mExecutor, mHandler, requireContext(),
                "file://" + tokensPath + "/" + logo, Integer.MIN_VALUE, mIvFavNetwork, null, true);
        AndroidUtils.show(mIvFavNetworkContainer);
        mNetworkName.setText(network.chainName);
    }
}
