/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.fragment.app.Fragment;

import org.chromium.brave_wallet.mojom.GetEncryptionPublicKeyRequest;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.CryptoModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class EncryptionKeyFragment extends Fragment implements View.OnClickListener {
    private static final String ACTIVITY_TYPE = "param1";
    private ImageView mAccountImage;
    private TextView mAccountName;
    private TextView mTvMessageDesc;
    private TextView mNetworkName;
    private Button mBtCancel;
    private Button mBtSign;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private BraveWalletDAppsActivity.ActivityType mActivityType;
    private WalletModel mWalletModel;
    private GetEncryptionPublicKeyRequest mEncryptionPublicKeyRequest;

    /**
     * Factory method
     *
     * @param activityType to define the type of public-encryption-key or decrypt-request.
     * @return A new instance of fragment EncryptionKeyFragment.
     */
    public static EncryptionKeyFragment newInstance(
            BraveWalletDAppsActivity.ActivityType activityType) {
        EncryptionKeyFragment fragment = new EncryptionKeyFragment();
        Bundle args = new Bundle();
        args.putSerializable(ACTIVITY_TYPE, activityType);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            mWalletModel = activity.getWalletModel();
        }
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        assert getArguments() != null;
        mActivityType = (BraveWalletDAppsActivity.ActivityType) getArguments().getSerializable(
                ACTIVITY_TYPE);
        View view = inflater.inflate(R.layout.fragment_encryption_key, container, false);
        mAccountImage = view.findViewById(R.id.fragment_encryption_msg_cv_iv_account);
        mAccountName = view.findViewById(R.id.fragment_encryption_msg_tv_account_name);
        mTvMessageDesc = view.findViewById(R.id.fragment_encryption_msg_desc);
        mNetworkName = view.findViewById(R.id.fragment_encryption_msg_tv_network_name);

        mBtCancel = view.findViewById(R.id.fragment_encryption_msg_btn_cancel);
        mBtSign = view.findViewById(R.id.fragment_encryption_msg_btn_sign);
        mBtSign.setOnClickListener(this);
        mBtCancel.setOnClickListener(this);
        initState();

        return view;
    }

    private void initState() {
        if (mWalletModel != null) {
            mWalletModel.getCryptoModel().getNetworkModel().mPairChainAndNetwork.observe(
                    getViewLifecycleOwner(), chainIdAndInfosPair -> {
                        String chainId = chainIdAndInfosPair.first;
                        NetworkInfo[] cryptoNetworks = chainIdAndInfosPair.second;
                        if (!TextUtils.isEmpty(chainId) && cryptoNetworks.length > 0) {
                            mNetworkName.setText(
                                    Utils.getNetworkText(requireActivity(), chainId, cryptoNetworks)
                                            .toString());
                        }
                    });

            mWalletModel.getKeyringModel().mSelectedAccount.observe(
                    getViewLifecycleOwner(), accountInfo -> {
                        Utils.setBlockiesBitmapResource(
                                mExecutor, mHandler, mAccountImage, accountInfo.address, true);
                        mAccountName.setText(accountInfo.name);
                    });
            mWalletModel.getCryptoModel().getPublicEncryptionRequest(encryptionPublicKeyRequest -> {
                if (encryptionPublicKeyRequest != null) {
                    mEncryptionPublicKeyRequest = encryptionPublicKeyRequest;
                    SpannableStringBuilder requestDescription = new SpannableStringBuilder();
                    requestDescription.append(
                            Utils.geteTLD(encryptionPublicKeyRequest.originInfo.eTldPlusOne));
                    requestDescription.append(" ");
                    requestDescription.append(
                            getString(R.string.brave_wallet_provide_encryption_key_description));
                    mTvMessageDesc.setText(requestDescription);
                }
            });
        }
    }

    @Override
    public void onClick(View v) {
        final int id = v.getId();
        if (id == R.id.fragment_encryption_msg_btn_sign) {
            mWalletModel.getCryptoModel().processPublicEncryptionKey(
                    true, mEncryptionPublicKeyRequest.originInfo.origin);
        } else if (id == R.id.fragment_encryption_msg_btn_cancel) {
            mWalletModel.getCryptoModel().processPublicEncryptionKey(
                    false, mEncryptionPublicKeyRequest.originInfo.origin);
        }
    }
}
