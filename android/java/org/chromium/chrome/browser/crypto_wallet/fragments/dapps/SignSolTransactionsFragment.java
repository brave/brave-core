/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.content.res.ColorStateList;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.core.content.ContextCompat;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;
import com.google.android.material.tabs.TabLayoutMediator;

import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.OriginInfo;
import org.chromium.brave_wallet.mojom.SignSolTransactionsRequest;
import org.chromium.brave_wallet.mojom.SolanaInstruction;
import org.chromium.brave_wallet.mojom.SolanaTxData;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.FragmentNavigationItemAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItem;
import org.chromium.chrome.browser.crypto_wallet.fragments.TwoLineItemFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.WalletBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.presenters.SolanaInstructionPresenter;
import org.chromium.chrome.browser.crypto_wallet.util.NavigationItem;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@NullMarked
public class SignSolTransactionsFragment extends WalletBottomSheetDialogFragment {
    private List<NavigationItem> mTabTitles;
    private List<SignSolTransactionsRequest> mSignSolTransactionRequests;
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
    private List<TwoLineItem> mDetails;
    private LinearLayout mWarningLl;
    private TextView mTxLearnMore;
    private TextView mTvTxCounter;
    private SignTx mSignTxStep = SignTx.SIGN_RISK;
    private int mTxRequestNumber;
    private SignSolTransactionsRequest mSignSolTransactionsRequest;
    private Button mBtnCounterNext;
    private List<SolanaTxData> mTxData;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        mDetails = new ArrayList<>();
        mTabTitles = new ArrayList<>();
        mTabTitles.add(
                new NavigationItem(getString(R.string.details), new TwoLineItemFragment(mDetails)));
        mSignSolTransactionRequests = Collections.emptyList();
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_sign_tx_message, container, false);
        mViewPager = view.findViewById(R.id.fragment_sign_tx_msg_tv_message_view_pager);
        mTabLayout = view.findViewById(R.id.fragment_sign_tx_msg_tv_message_tabs);
        mAccountImage = view.findViewById(R.id.fragment_sign_tx_msg_cv_iv_account);
        mAccountName = view.findViewById(R.id.fragment_sign_tx_msg_tv_account_name);
        mNetworkName = view.findViewById(R.id.fragment_sign_tx_msg_tv_network_name);
        mViewPager.setUserInputEnabled(false);
        FragmentNavigationItemAdapter adapter = new FragmentNavigationItemAdapter(this, mTabTitles);
        mViewPager.setAdapter(adapter);
        new TabLayoutMediator(
                        mTabLayout,
                        mViewPager,
                        (tab, position) -> tab.setText(mTabTitles.get(position).getTitle()))
                .attach();

        mTvTxCounter = view.findViewById(R.id.fragment_sign_tx_msg_tv_counter);
        mBtnCounterNext = view.findViewById(R.id.fragment_encryption_msg_btn_next);
        mWarningLl = view.findViewById(R.id.fragment_sign_tx_warning_container);
        mTxLearnMore = view.findViewById(R.id.fragment_sign_tx_warning_learn_more);
        mBtCancel = view.findViewById(R.id.fragment_sign_tx_msg_btn_cancel);
        mBtSign = view.findViewById(R.id.fragment_sign_tx_msg_btn_sign);
        mWebSite = view.findViewById(R.id.domain);
        initComponents();

        return view;
    }

    private void initComponents() {
        updateTxPanelPerStep();
        fetchSignRequestData();
        Spanned associatedSPLTokenAccountInfo =
                Utils.createSpanForSurroundedPhrase(
                        requireContext(),
                        R.string.learn_more,
                        (v) -> {
                            TabUtils.openUrlInNewTab(
                                    false, WalletConstants.URL_SIGN_TRANSACTION_REQUEST);
                            TabUtils.bringChromeTabbedActivityToTheTop(getActivity());
                        });
        mTxLearnMore.setMovementMethod(LinkMovementMethod.getInstance());
        mTxLearnMore.setText(associatedSPLTokenAccountInfo);

        mBtSign.setOnClickListener(v -> processRequest(true));
        mBtCancel.setOnClickListener(v -> processRequest(false));

        mTvTxCounter.setOnClickListener(
                v -> {
                    incrementCounter();
                    updateSignDataAndDetails();
                });
        mBtnCounterNext.setOnClickListener(
                v -> {
                    incrementCounter();
                    updateSignDataAndDetails();
                });
    }

    // Update tx counter UI based on the current value of mTxRequestNumber
    private void updateSignRequestData() {
        if (mSignSolTransactionRequests.isEmpty()) {
            return;
        }

        if (mTxRequestNumber >= mSignSolTransactionRequests.size()) {
            mTxRequestNumber = 0;
        }
        mSignSolTransactionsRequest = mSignSolTransactionRequests.get(mTxRequestNumber);
        mTxData = TransactionUtils.safeSolData(mSignSolTransactionsRequest);
        if (mSignSolTransactionRequests.size() == 1) {
            mTvTxCounter.setVisibility(View.GONE);
            mBtnCounterNext.setVisibility(View.GONE);
        } else {
            if (mSignTxStep != SignTx.SIGN_RISK) {
                mTvTxCounter.setVisibility(View.VISIBLE);
                mBtnCounterNext.setVisibility(View.VISIBLE);
            }
        }
        mTvTxCounter.setText(
                getString(
                        R.string.brave_wallet_queue_of,
                        (mTxRequestNumber + 1),
                        mSignSolTransactionRequests.size()));
        updateActionState(mTxRequestNumber == 0);
        updateAccount(mSignSolTransactionsRequest.fromAccountId);
        updateNetwork(
                mSignSolTransactionsRequest.chainId.coin,
                mSignSolTransactionsRequest.chainId.chainId);
        OriginInfo originInfo = mSignSolTransactionsRequest.originInfo;
        if (originInfo != null && URLUtil.isValidUrl(originInfo.originSpec)) {
            mWebSite.setVisibility(View.VISIBLE);
            mWebSite.setText(Utils.geteTldSpanned(originInfo));
        }
    }

    private void updateSignDetails() {
        if (mTxData == null || mTxData.isEmpty()) {
            return;
        }
        mDetails.clear();

        for (SolanaTxData txData : mTxData) {
            for (SolanaInstruction solanaInstruction : txData.instructions) {
                SolanaInstructionPresenter solanaInstructionPresenter =
                        new SolanaInstructionPresenter(solanaInstruction);
                mDetails.addAll(solanaInstructionPresenter.toTwoLineList(requireContext()));
                if (mTxData.size() > 1 || txData.instructions.length > 1) {
                    mDetails.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemDivider());
                }
            }
        }
        for (NavigationItem navigationItem : mTabTitles) {
            ((TwoLineItemFragment) navigationItem.getFragment()).invalidateData();
        }
    }

    private void incrementCounter() {
        if (mSignSolTransactionRequests.isEmpty()) {
            return;
        }
        if (mTxRequestNumber >= mSignSolTransactionRequests.size()) {
            mTxRequestNumber = 0;
            return;
        }
        mTxRequestNumber++;
    }

    private void updateActionState(final boolean isEnabled) {
        if (mSignTxStep == SignTx.SIGN_RISK) return;
        mBtCancel.setEnabled(isEnabled);
        mBtSign.setEnabled(isEnabled);
        mBtSign.setBackgroundTintList(
                ColorStateList.valueOf(
                        ContextCompat.getColor(
                                requireContext(),
                                isEnabled
                                        ? R.color.brave_action_color
                                        : R.color.baseline_neutral_30)));
    }

    private void fetchSignRequestData() {
        getWalletModel()
                .getDappsModel()
                .fetchSignSolTransactionsRequests()
                .observe(
                        getViewLifecycleOwner(),
                        requests -> {
                            if (requests.isEmpty()) return;
                            mSignSolTransactionRequests = requests;
                            updateSignDataAndDetails();
                        });
    }

    private void updateTxPanelPerStep() {
        if (mSignTxStep == SignTx.SIGN_RISK) {
            mViewPager.setVisibility(View.GONE);
            mTvTxCounter.setVisibility(View.GONE);
            mBtnCounterNext.setVisibility(View.GONE);
            mWarningLl.setVisibility(View.VISIBLE);
            mBtSign.setBackgroundTintList(
                    ColorStateList.valueOf(
                            ContextCompat.getColor(requireContext(), R.color.brave_theme_error)));
            mBtSign.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
            mBtSign.setText(R.string.continue_text);
        } else if (mSignTxStep == SignTx.SIGN_TX) {
            mWarningLl.setVisibility(View.GONE);
            mViewPager.setVisibility(View.VISIBLE);
            mBtSign.setBackgroundTintList(
                    ColorStateList.valueOf(
                            ContextCompat.getColor(requireContext(), R.color.brave_action_color)));
            mBtSign.setCompoundDrawablesRelativeWithIntrinsicBounds(R.drawable.ic_key, 0, 0, 0);
            mBtSign.setText(R.string.brave_wallet_sign_message_positive_button_action);
        }
    }

    private void processRequest(boolean isApproved) {
        if (isApproved && mSignTxStep == SignTx.SIGN_RISK) {
            // Continue clicked, update ui
            mSignTxStep = SignTx.SIGN_TX;
            updateTxPanelPerStep();
            updateSignDataAndDetails();
            return;
        }

        getWalletModel()
                .getDappsModel()
                .notifySignSolTransactionsRequestProcessed(isApproved, mSignSolTransactionsRequest);
    }

    private void updateAccount(AccountId fromAccountId) {
        if (fromAccountId == null) {
            return;
        }
        assert (fromAccountId.coin == CoinType.SOL);
        getKeyringModel()
                .getAccounts(
                        accountInfos -> {
                            AccountInfo accountInfo =
                                    Utils.findAccount(accountInfos, fromAccountId);
                            if (accountInfo == null) {
                                return;
                            }
                            assert (accountInfo.address != null);

                            Utils.setBlockiesBitmapResourceFromAccount(
                                    mExecutor, mHandler, mAccountImage, accountInfo, true);
                            String accountText = accountInfo.name + "\n" + accountInfo.address;
                            mAccountName.setText(accountText);
                        });
    }

    private void updateNetwork(@CoinType.EnumType int coin, String chainId) {
        mNetworkName.setText("");

        LiveDataUtil.observeOnce(
                getWalletModel().getCryptoModel().getNetworkModel().mCryptoNetworks,
                allNetworks -> {
                    for (NetworkInfo networkInfo : allNetworks) {
                        if (networkInfo.coin == coin && networkInfo.chainId.equals(chainId)) {
                            mNetworkName.setText(networkInfo.chainName);
                            break;
                        }
                    }
                });
    }

    private void updateSignDataAndDetails() {
        updateSignRequestData();
        updateSignDetails();
    }

    private enum SignTx {
        SIGN_RISK,
        SIGN_TX
    }
}
