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

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.OriginInfo;
import org.chromium.brave_wallet.mojom.SignAllTransactionsRequest;
import org.chromium.brave_wallet.mojom.SignTransactionRequest;
import org.chromium.brave_wallet.mojom.SolanaInstruction;
import org.chromium.brave_wallet.mojom.SolanaTxData;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity.ActivityType;
import org.chromium.chrome.browser.crypto_wallet.adapters.FragmentNavigationItemAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItem;
import org.chromium.chrome.browser.crypto_wallet.fragments.TwoLineItemFragment;
import org.chromium.chrome.browser.crypto_wallet.presenters.SolanaInstructionPresenter;
import org.chromium.chrome.browser.crypto_wallet.util.NavigationItem;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class SignTransactionFragment extends BaseDAppsBottomSheetDialogFragment {
    private static final String TAG = "SignTransaction";

    private static final String PARAM_ACITITY_TYPE = "sign_param";
    private List<NavigationItem> mTabTitles;
    private List<SignTransactionRequest> mSignTransactionRequests;
    private List<SignAllTransactionsRequest> mSignAllTransactionRequests;
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
    private List<TwoLineItem> details;
    private LinearLayout mWarningLl;
    private TextView mTxLearnMore;
    private TextView mTvTxCounter;
    private ActivityType mActivityType;
    private WalletModel mWalletModel;
    private SignTx mSignTxStep = SignTx.SIGN_RISK;
    private int mTxRequestNumber;
    private SignTransactionRequest mSignTransactionRequest;
    private SignAllTransactionsRequest mSignAllTransactionsRequest;
    private Button mBtnCounterNext;
    private List<SolanaTxData> mTxDatas;
    private final View.OnClickListener onNextTxClick =
            v -> {
                incrementCounter();
                updateSignDataAndDetails();
            };

    public static SignTransactionFragment newInstance(ActivityType activityType) {
        SignTransactionFragment fragment = new SignTransactionFragment();
        Bundle args = new Bundle();
        args.putSerializable(PARAM_ACITITY_TYPE, activityType);
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
        details = new ArrayList<>();
        mTabTitles = new ArrayList<>();
        mTabTitles.add(
                new NavigationItem(getString(R.string.details), new TwoLineItemFragment(details)));
        mSignTransactionRequests = Collections.emptyList();
        mSignAllTransactionRequests = Collections.emptyList();
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate " + e);
        }
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
        assert getArguments() != null;
        mActivityType = (ActivityType) getArguments().getSerializable(PARAM_ACITITY_TYPE);
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

        mBtSign.setOnClickListener(
                v -> {
                    processRequest(true);
                });
        mBtCancel.setOnClickListener(
                v -> {
                    processRequest(false);
                });
        mTvTxCounter.setOnClickListener(onNextTxClick);
        mBtnCounterNext.setOnClickListener(onNextTxClick);
    }

    // Update tx counter UI based on the current value of mTxRequestNumber
    private void updateSignRequestData() {
        if (!mSignTransactionRequests.isEmpty() && !mSignAllTransactionRequests.isEmpty()) {
            return;
        }
        int size = 0;
        AccountId fromAccountId = null;
        @CoinType.EnumType int coin = CoinType.SOL;
        String chainId = null;
        OriginInfo originInfo = null;
        switch (mActivityType) {
            case SIGN_TRANSACTION:
                if (mTxRequestNumber >= mSignTransactionRequests.size()) {
                    mTxRequestNumber = 0;
                }
                mSignTransactionRequest = mSignTransactionRequests.get(mTxRequestNumber);
                size = mSignTransactionRequests.size();
                mTxDatas = Arrays.asList(TransactionUtils.safeSolData(mSignTransactionRequest));
                coin = mSignTransactionRequest.coin;
                fromAccountId = mSignTransactionRequest.fromAccountId;
                originInfo = mSignTransactionRequest.originInfo;
                chainId = mSignTransactionRequest.chainId;
                break;
            case SIGN_ALL_TRANSACTIONS:
                if (mTxRequestNumber >= mSignAllTransactionRequests.size()) {
                    mTxRequestNumber = 0;
                }
                mSignAllTransactionsRequest = mSignAllTransactionRequests.get(mTxRequestNumber);
                size = mSignAllTransactionRequests.size();
                mTxDatas = TransactionUtils.safeSolData(mSignAllTransactionsRequest);
                coin = mSignAllTransactionsRequest.coin;
                fromAccountId = mSignAllTransactionsRequest.fromAccountId;
                originInfo = mSignAllTransactionsRequest.originInfo;
                chainId = mSignAllTransactionsRequest.chainId;
                break;
            default: // Do nothing
        }
        if (size == 1) {
            mTvTxCounter.setVisibility(View.GONE);
            mBtnCounterNext.setVisibility(View.GONE);
        } else {
            if (mSignTxStep != SignTx.SIGN_RISK) {
                mTvTxCounter.setVisibility(View.VISIBLE);
                mBtnCounterNext.setVisibility(View.VISIBLE);
            }
        }
        mTvTxCounter.setText(
                getString(R.string.brave_wallet_queue_of, (mTxRequestNumber + 1), size));
        updateActionState(mTxRequestNumber == 0);
        updateAccount(fromAccountId);
        updateNetwork(coin, chainId);
        if (originInfo != null && URLUtil.isValidUrl(originInfo.originSpec)) {
            mWebSite.setVisibility(View.VISIBLE);
            mWebSite.setText(Utils.geteTldSpanned(originInfo));
        }
    }

    private void updateSignDetails() {
        if (mTxDatas == null || mTxDatas.isEmpty()) {
            return;
        }
        details.clear();

        for (SolanaTxData txData : mTxDatas) {
            for (SolanaInstruction solanaInstruction : txData.instructions) {
                SolanaInstructionPresenter solanaInstructionPresenter =
                        new SolanaInstructionPresenter(solanaInstruction);
                details.addAll(solanaInstructionPresenter.toTwoLineList(requireContext()));
                if (mTxDatas.size() > 1 || txData.instructions.length > 1) {
                    details.add(new TwoLineItemRecyclerViewAdapter.TwoLineItemDivider());
                }
            }
        }
        for (NavigationItem navigationItem : mTabTitles) {
            ((TwoLineItemFragment) navigationItem.getFragment()).invalidateData();
        }
    }

    private void incrementCounter() {
        switch (mActivityType) {
            case SIGN_TRANSACTION:
                if (mSignTransactionRequests.size() == 0) return;
                if (mTxRequestNumber >= mSignTransactionRequests.size()) {
                    mTxRequestNumber = 0;
                    return;
                }
                break;
            case SIGN_ALL_TRANSACTIONS:
                if (mSignAllTransactionRequests.size() == 0) return;
                if (mTxRequestNumber >= mSignAllTransactionRequests.size()) {
                    mTxRequestNumber = 0;
                    return;
                }
                break;
            default: // Do nothing
        }
        mTxRequestNumber++;
    }

    private void updateActionState(boolean isEnabled) {
        if (mSignTxStep == SignTx.SIGN_RISK) return;
        mBtCancel.setEnabled(isEnabled);
        mBtSign.setEnabled(isEnabled);
        if (isEnabled) {
            mBtSign.setBackgroundTintList(
                    ColorStateList.valueOf(
                            ContextCompat.getColor(requireContext(), R.color.brave_action_color)));

        } else {
            mBtSign.setBackgroundTintList(
                    ColorStateList.valueOf(
                            ContextCompat.getColor(requireContext(), R.color.modern_grey_700)));
        }
    }

    private void fetchSignRequestData() {
        switch (mActivityType) {
            case SIGN_TRANSACTION:
                mWalletModel
                        .getDappsModel()
                        .fetchSignTxRequest()
                        .observe(
                                getViewLifecycleOwner(),
                                requests -> {
                                    if (requests.size() == 0) return;
                                    mSignTransactionRequests = requests;
                                    updateSignDataAndDetails();
                                });
                break;
            case SIGN_ALL_TRANSACTIONS:
                mWalletModel
                        .getDappsModel()
                        .fetchSignAllTxRequest()
                        .observe(
                                getViewLifecycleOwner(),
                                requests -> {
                                    if (requests.size() == 0) return;
                                    mSignAllTransactionRequests = requests;
                                    updateSignDataAndDetails();
                                });
                break;
            default: // Do nothing
        }
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
        switch (mActivityType) {
            case SIGN_TRANSACTION:
                mWalletModel.getDappsModel().signTxRequest(isApproved, mSignTransactionRequest);
                break;
            case SIGN_ALL_TRANSACTIONS:
                mWalletModel
                        .getDappsModel()
                        .signAllTxRequest(isApproved, mSignAllTransactionsRequest);
                break;
            default: // Do nothing
        }
    }

    private void updateAccount(AccountId fromAccountId) {
        if (fromAccountId == null) {
            return;
        }
        assert (fromAccountId.coin == CoinType.SOL);
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.getWalletModel()
                    .getKeyringModel()
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
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "updateAccount " + e);
        }
    }

    private void updateNetwork(@CoinType.EnumType int coin, String chainId) {
        mNetworkName.setText("");

        if (chainId == null) {
            return;
        }
        LiveDataUtil.observeOnce(
                mWalletModel.getCryptoModel().getNetworkModel().mCryptoNetworks,
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
