/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.SignMessagePagerAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class SignMessageFragment extends BaseDAppsBottomSheetDialogFragment {
    private List<String> mTabTitles;
    private SignMessageRequest mCurrentSignMessageRequest;
    private SignMessagePagerAdapter mSignMessagePagerAdapter;
    private ViewPager2 mViewPager;
    private TabLayout mTabLayout;
    private ImageView mAccountImage;
    private TextView mAccountName;
    private TextView mNetworkName;
    private Button mBtCancel;
    private Button mBtSign;
    private TextView mWebSite;
    private ExecutorService mExecutor;
    private Handler mHandler;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mTabTitles = new ArrayList<>();
        mTabTitles.add(getString(R.string.details));
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_sign_message, container, false);
        mViewPager = view.findViewById(R.id.fragment_sign_msg_tv_message_view_pager);
        mTabLayout = view.findViewById(R.id.fragment_sign_msg_tv_message_tabs);
        mAccountImage = view.findViewById(R.id.fragment_sign_msg_cv_iv_account);
        mAccountName = view.findViewById(R.id.fragment_sign_msg_tv_account_name);
        mNetworkName = view.findViewById(R.id.fragment_sign_msg_tv_network_name);
        mViewPager.setUserInputEnabled(false);

        mBtCancel = view.findViewById(R.id.fragment_sign_msg_btn_cancel);
        mBtSign = view.findViewById(R.id.fragment_sign_msg_btn_sign);
        mWebSite = view.findViewById(R.id.domain);
        initComponents();

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
            mSignMessagePagerAdapter =
                    new SignMessagePagerAdapter(this, mTabTitles, mCurrentSignMessageRequest);

            mViewPager.setAdapter(mSignMessagePagerAdapter);
            new TabLayoutMediator(mTabLayout, mViewPager,
                    (tab, position) -> tab.setText(mTabTitles.get(position)))
                    .attach();
            if (init) {
                mBtCancel.setOnClickListener(v -> { notifySignMessageRequestProcessed(false); });
                mBtSign.setOnClickListener(v -> { notifySignMessageRequestProcessed(true); });
            }
            if (mCurrentSignMessageRequest.originInfo != null
                    && URLUtil.isValidUrl(mCurrentSignMessageRequest.originInfo.originSpec)) {
                mWebSite.setText(
                        Utils.geteTLD(new GURL(mCurrentSignMessageRequest.originInfo.originSpec),
                                mCurrentSignMessageRequest.originInfo.eTldPlusOne));
            }
            updateAccount(mCurrentSignMessageRequest.address);
            updateNetwork(mCurrentSignMessageRequest.coin);
        });
    }

    private void updateAccount(String address) {
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            activity.getWalletModel().getKeyringModel().getAccounts(accountInfos -> {
                if (address == null) return;
                AccountInfo accountInfo = Utils.findAccount(accountInfos, address);
                String accountText = (accountInfo != null ? accountInfo.name + "\n" : "") + address;
                Utils.setBlockiesBitmapResource(mExecutor, mHandler, mAccountImage, address, true);
                mAccountName.setText(accountText);
            });
        }
    }

    private void updateNetwork(@CoinType.EnumType int coin) {
        getJsonRpcService().getNetwork(
                coin, selectedNetwork -> { mNetworkName.setText(selectedNetwork.chainName); });
    }
}
