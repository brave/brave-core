/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.SignMessagePagerAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Fragment used by DApps sign operation */
public class SignMessageFragment extends BaseDAppsBottomSheetDialogFragment {
    private static final String TAG = "SignMessageFragment";

    private List<String> mTabTitles;
    private SignMessageRequest mCurrentSignMessageRequest;
    private SignMessagePagerAdapter mSignMessagePagerAdapter;
    private ViewPager2 mViewPager;
    private TabLayout mTabLayout;
    private ImageView mAccountImage;
    private TextView mAccountName;
    private TextView mNetworkName;
    private Button mBtCancel;
    protected Button mBtSign;
    private TextView mWebSite;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private WalletModel mWalletModel;

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
        getBraveWalletService()
                .notifySignMessageRequestProcessed(
                        approved, mCurrentSignMessageRequest.id, null, null);
        fillSignMessageInfo(false);
    }

    private void initComponents() {
        fillSignMessageInfo(true);
    }

    private void fillSignMessageInfo(boolean init) {
        getBraveWalletService()
                .getPendingSignMessageRequests(
                        requests -> {
                            if (requests == null || requests.length == 0) {
                                Intent intent = new Intent();
                                getActivity().setResult(Activity.RESULT_OK, intent);
                                getActivity().finish();

                                return;
                            }

                            mCurrentSignMessageRequest = requests[0];
                            mSignMessagePagerAdapter =
                                    new SignMessagePagerAdapter(
                                            this, mTabTitles, mCurrentSignMessageRequest);

                            mViewPager.setAdapter(mSignMessagePagerAdapter);
                            new TabLayoutMediator(
                                            mTabLayout,
                                            mViewPager,
                                            (tab, position) ->
                                                    tab.setText(mTabTitles.get(position)))
                                    .attach();
                            if (init) {
                                mBtCancel.setOnClickListener(
                                        v -> {
                                            notifySignMessageRequestProcessed(false);
                                        });
                                mBtSign.setOnClickListener(
                                        v -> {
                                            notifySignMessageRequestProcessed(true);
                                        });
                            }
                            if (mCurrentSignMessageRequest.originInfo != null
                                    && URLUtil.isValidUrl(
                                            mCurrentSignMessageRequest.originInfo.originSpec)) {
                                mWebSite.setText(
                                        Utils.geteTldSpanned(
                                                mCurrentSignMessageRequest.originInfo));
                            }
                            updateAccount(mCurrentSignMessageRequest.accountId);
                            updateNetwork(mCurrentSignMessageRequest.chainId);
                        });
    }

    private void updateAccount(AccountId accountId) {
        if (accountId == null) {
            return;
        }
        assert (accountId.coin == CoinType.ETH || accountId.coin == CoinType.SOL);

        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.getWalletModel()
                    .getKeyringModel()
                    .getAccounts(
                            accountInfos -> {
                                AccountInfo accountInfo =
                                        Utils.findAccount(accountInfos, accountId);
                                if (accountInfo == null) {
                                    return;
                                }
                                assert (accountInfo.address != null);

                                Utils.setBlockiesBitmapResourceFromAccount(
                                        mExecutor, mHandler, mAccountImage, accountInfo, true);
                                String accountText = accountInfo.name + "\n" + accountInfo.address;
                                mAccountName.setText(accountText);
                            });
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "updateAccount " + e);
        }
    }

    private void updateNetwork(String chainId) {
        if (JavaUtils.anyNull(mWalletModel, chainId)) return;
        var selectedNetwork = mWalletModel.getNetworkModel().getNetwork(chainId);
        mNetworkName.setText(selectedNetwork.chainName);
    }
}
