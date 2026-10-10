/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;
import android.widget.Button;
import android.widget.TextView;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AddSuggestTokenRequest;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.OriginInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class AddTokenFragment extends BaseDAppsFragment {
    private static final String TAG = "AddTokenFragment";
    private AddSuggestTokenRequest mCurrentAddSuggestTokenRequest;
    private Button mBtCancel;
    private Button mBtAdd;
    private TextView mNetworkName;
    private TextView mWebSite;
    private TextView mTokenName;
    private TextView mTokenAddress;
    private WalletModel mWalletModel;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_add_token, container, false);

        mBtCancel = view.findViewById(R.id.fragment_add_btn_cancel);
        mBtAdd = view.findViewById(R.id.fragment_add_btn_add);
        mNetworkName = view.findViewById(R.id.fragment_add_token_tv_network_name);
        mWebSite = view.findViewById(R.id.fragment_add_token_tv_web_site);
        mTokenName = view.findViewById(R.id.fragment_add_token_tv_token);
        mTokenAddress = view.findViewById(R.id.fragment_add_token_tv_address);
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate ", e);
        }
        initComponents(true);

        return view;
    }

    private void initComponents(boolean init) {
        fillAddSuggestTokenRequest(init);
    }

    private void notifyAddSuggestTokenRequestProcessed(boolean approved) {
        String[] addresses = new String[] {mCurrentAddSuggestTokenRequest.token.contractAddress};
        getBraveWalletService().notifyAddSuggestTokenRequestsProcessed(approved, addresses);
        initComponents(false);
    }

    private void fillAddSuggestTokenRequest(boolean init) {
        if (mWalletModel == null) return;
        getBraveWalletService()
                .getPendingAddSuggestTokenRequests(
                        requests -> {
                            if (requests == null || requests.length == 0) {
                                Intent intent = new Intent();
                                getActivity().setResult(Activity.RESULT_OK, intent);
                                getActivity().finish();

                                return;
                            }
                            mCurrentAddSuggestTokenRequest = requests[0];
                            NetworkInfo selectedNetwork =
                                    mWalletModel
                                            .getNetworkModel()
                                            .getNetwork(
                                                    mCurrentAddSuggestTokenRequest.token.chainId);
                            if (init) {
                                mBtCancel.setOnClickListener(
                                        v -> {
                                            notifyAddSuggestTokenRequestProcessed(false);
                                        });
                                mBtAdd.setOnClickListener(
                                        v -> {
                                            notifyAddSuggestTokenRequestProcessed(true);
                                        });
                                mTokenAddress.setOnClickListener(
                                        v -> {
                                            Activity activity = getActivity();
                                            if (activity instanceof BraveWalletBaseActivity
                                                    && selectedNetwork != null) {
                                                Utils.openAddress(
                                                        "/token/"
                                                                + mCurrentAddSuggestTokenRequest
                                                                        .token
                                                                        .contractAddress,
                                                        (BraveWalletBaseActivity) activity,
                                                        mCurrentAddSuggestTokenRequest.token.coin,
                                                        selectedNetwork);
                                            }
                                        });
                            }
                            fillOriginInfo(mCurrentAddSuggestTokenRequest.origin);
                            initToken();
                            updateNetwork(selectedNetwork);
                        });
    }

    private void fillOriginInfo(OriginInfo originInfo) {
        if (originInfo != null && URLUtil.isValidUrl(originInfo.originSpec)) {
            mWebSite.setText(Utils.geteTldSpanned(originInfo));
        }
    }

    private void updateNetwork(NetworkInfo networkInfo) {
        if (JavaUtils.anyNull(mWalletModel, networkInfo)) return;
        mNetworkName.setText(networkInfo.chainName);
    }

    private void initToken() {
        String tokenName = mCurrentAddSuggestTokenRequest.token.name;
        if (tokenName.isEmpty()) {
            tokenName = mCurrentAddSuggestTokenRequest.token.symbol;
        } else {
            tokenName += " (" + mCurrentAddSuggestTokenRequest.token.symbol + ")";
        }
        mTokenName.setText(tokenName);
        mTokenAddress.setText(
                Utils.stripAccountAddress(mCurrentAddSuggestTokenRequest.token.contractAddress));
    }
}
