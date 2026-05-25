/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.app.Activity;
import android.content.Intent;
import android.content.res.ColorStateList;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Html;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.core.content.ContextCompat;

import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.SignDataUnion;
import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.WalletBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.Validations;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.util.TabUtils;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Fragment used by DApps sign operation */
@NullMarked
public class SignMessageFragment extends WalletBottomSheetDialogFragment {
    /**
     * Two-step flow shown for sign requests that need a risk acknowledgment
     * (EIP-712 typed data and Cardano payloads). Other request types start
     * directly at {@link #SIGN_TX}.
     */
    private enum SignStep {
        // Risk warning is displayed; the primary button reads "Continue".
        SIGN_RISK,
        // Message body is displayed; the primary button signs the request.
        SIGN_TX
    }

    private final ExecutorService mExecutor;
    private final Handler mHandler;

    @MonotonicNonNull private SignMessageRequest mCurrentSignMessageRequest;
    private boolean mUnicodeEscapeVersion;
    private TextView mSignMessageText;
    private ImageView mAccountImage;
    private TextView mAccountName;
    private TextView mNetworkName;
    private Button mBtCancel;
    protected Button mBtSign;
    private TextView mWebSite;
    private LinearLayout mWarningContainer;
    private LinearLayout mNonAsciiWarningLayout;
    private boolean mHasUnicodeWarning;
    private SignStep mSignStep = SignStep.SIGN_TX;

    public SignMessageFragment() {
        super();
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_sign_message, container, false);
        mSignMessageText = view.findViewById(R.id.sign_message_text);
        mAccountImage = view.findViewById(R.id.fragment_sign_msg_cv_iv_account);
        mAccountName = view.findViewById(R.id.fragment_sign_msg_tv_account_name);
        mNetworkName = view.findViewById(R.id.fragment_sign_msg_tv_network_name);

        mBtCancel = view.findViewById(R.id.fragment_sign_msg_btn_cancel);
        mBtSign = view.findViewById(R.id.fragment_sign_msg_btn_sign);
        mWebSite = view.findViewById(R.id.domain);
        mWarningContainer = view.findViewById(R.id.sign_msg_warning_container);
        mNonAsciiWarningLayout = view.findViewById(R.id.non_ascii_warning_layout);

        TextView warningLearnMore = view.findViewById(R.id.sign_warning_learn_more);
        warningLearnMore.setOnClickListener(
                v -> {
                    TabUtils.openUrlInNewTab(
                            false, WalletConstants.URL_SIGN_TRANSACTION_REQUEST);
                    TabUtils.bringChromeTabbedActivityToTheTop(getActivity());
                });

        mBtCancel.setOnClickListener(v -> notifySignMessageRequestProcessed(false));
        mBtSign.setOnClickListener(v -> onSignButtonClicked());

        fillSignMessageInfo();
        return view;
    }

    private void onSignButtonClicked() {
        if (mSignStep == SignStep.SIGN_RISK) {
            mSignStep = SignStep.SIGN_TX;
            updateWarningLayoutForCurrentStep();
            return;
        }
        notifySignMessageRequestProcessed(true);
    }

    private void notifySignMessageRequestProcessed(final boolean approved) {
        if (mCurrentSignMessageRequest != null) {
            getBraveWalletService()
                    .notifySignMessageRequestProcessed(
                            approved, mCurrentSignMessageRequest.id, null, null);
            fillSignMessageInfo();
        }
    }

    private void fillSignMessageInfo() {
        getBraveWalletService().getPendingSignMessageRequests(this::maybeHandlePendingRequests);
    }

    private void maybeHandlePendingRequests(final SignMessageRequest @Nullable [] requests) {
        if (requests == null || requests.length == 0) {
            Intent intent = new Intent();
            getActivity().setResult(Activity.RESULT_OK, intent);
            getActivity().finish();
            return;
        }

        mCurrentSignMessageRequest = requests[0];

        // Extract message and set up UI.
        mUnicodeEscapeVersion = false;
        final String message;
        final boolean isEip712;
        if (mCurrentSignMessageRequest.signData.which() == SignDataUnion.Tag.EthStandardSignData) {
            message = mCurrentSignMessageRequest.signData.getEthStandardSignData().message;
            isEip712 = false;
        } else if (mCurrentSignMessageRequest.signData.which()
                == SignDataUnion.Tag.EthSignTypedData) {
            message = mCurrentSignMessageRequest.signData.getEthSignTypedData().messageJson;
            isEip712 = true;
        } else if (mCurrentSignMessageRequest.signData.which()
                == SignDataUnion.Tag.SolanaSignData) {
            message = mCurrentSignMessageRequest.signData.getSolanaSignData().message;
            isEip712 = false;
        } else if (mCurrentSignMessageRequest.signData.which() == SignDataUnion.Tag.EthSiweData) {
            message = mCurrentSignMessageRequest.signData.getEthSiweData().statement;
            isEip712 = false;
        } else if (mCurrentSignMessageRequest.signData.which()
                == SignDataUnion.Tag.CardanoSignData) {
            message = mCurrentSignMessageRequest.signData.getCardanoSignData().message;
            isEip712 = false;
        } else {
            message = "";
            isEip712 = false;
        }

        updateTextEthSign(
                mCurrentSignMessageRequest.signData,
                mUnicodeEscapeVersion,
                assumeNonNull(message),
                isEip712);

        mHasUnicodeWarning = Validations.hasUnicode(message);
        if (mHasUnicodeWarning) {
            mSignMessageText.setLines(12);
            View view = getView();
            if (view != null) {
                TextView warningLinkText = view.findViewById(R.id.non_ascii_warning_text_link);
                warningLinkText.setOnClickListener(
                        v -> {
                            warningLinkText.setText(
                                    mUnicodeEscapeVersion
                                            ? getString(
                                                    R.string.wallet_non_ascii_characters_original)
                                            : getString(
                                                    R.string.wallet_non_ascii_characters_ascii));
                            mUnicodeEscapeVersion = !mUnicodeEscapeVersion;
                            updateTextEthSign(
                                    mCurrentSignMessageRequest.signData,
                                    mUnicodeEscapeVersion,
                                    message,
                                    isEip712);
                        });
            }
        }

        mSignStep = needsSignRiskWarning(mCurrentSignMessageRequest.signData)
                ? SignStep.SIGN_RISK
                : SignStep.SIGN_TX;
        updateWarningLayoutForCurrentStep();

        if (mCurrentSignMessageRequest.originInfo != null
                && URLUtil.isValidUrl(mCurrentSignMessageRequest.originInfo.originSpec)) {
            mWebSite.setText(Utils.geteTldSpanned(mCurrentSignMessageRequest.originInfo));
        }
        updateAccount(mCurrentSignMessageRequest.accountId);
        updateNetwork(mCurrentSignMessageRequest.chainId);
    }

    /**
     * EIP-712 typed data can grant broad authority (e.g. ERC-20 Permit
     * allowances) that is easy to misread, and Cardano sign payloads are opaque
     * to Brave; warn the user before either is signed.
     */
    private static boolean needsSignRiskWarning(final SignDataUnion signData) {
        int tag = signData.which();
        return tag == SignDataUnion.Tag.EthSignTypedData
                || tag == SignDataUnion.Tag.CardanoSignData;
    }

    private void updateWarningLayoutForCurrentStep() {
        if (mSignStep == SignStep.SIGN_RISK) {
            mWarningContainer.setVisibility(View.VISIBLE);
            mSignMessageText.setVisibility(View.GONE);
            mNonAsciiWarningLayout.setVisibility(View.GONE);
            mBtSign.setBackgroundTintList(
                    ColorStateList.valueOf(
                            ContextCompat.getColor(requireContext(), R.color.brave_theme_error)));
            mBtSign.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
            mBtSign.setText(R.string.continue_text);
        } else {
            mWarningContainer.setVisibility(View.GONE);
            mSignMessageText.setVisibility(View.VISIBLE);
            mNonAsciiWarningLayout.setVisibility(mHasUnicodeWarning ? View.VISIBLE : View.GONE);
            mBtSign.setBackgroundTintList(
                    ColorStateList.valueOf(
                            ContextCompat.getColor(requireContext(), R.color.brave_action_color)));
            mBtSign.setCompoundDrawablesRelativeWithIntrinsicBounds(R.drawable.ic_key, 0, 0, 0);
            mBtSign.setText(R.string.brave_wallet_sign_message_positive_button_action);
        }
    }

    private void updateAccount(AccountId accountId) {
        assert (accountId.coin == CoinType.ETH
                || accountId.coin == CoinType.SOL
                || accountId.coin == CoinType.ADA);
        getKeyringModel()
                .getAccounts(
                        accountInfos -> {
                            AccountInfo accountInfo = Utils.findAccount(accountInfos, accountId);
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

    private void updateNetwork(String chainId) {
        NetworkInfo selectedNetwork = getWalletModel().getNetworkModel().getNetwork(chainId);
        mNetworkName.setText(selectedNetwork.chainName);
    }

    private void updateTextEthSign(
            final SignDataUnion signData,
            final boolean unicodeEscape,
            final String message,
            final boolean isEip712) {
        String escapedDomain = "";
        if (isEip712) {
            String domain = signData.getEthSignTypedData().domainJson;
            escapedDomain = unicodeEscape ? Validations.unicodeEscape(domain) : domain;
        }
        String escapedMessage = unicodeEscape ? Validations.unicodeEscape(message) : message;

        Spanned domainPart =
                escapedDomain.isEmpty()
                        ? new SpannableString("")
                        : AndroidUtils.formatHTML(
                                getString(
                                        R.string.wallet_sign_message_details_domain_section,
                                        Html.escapeHtml(escapedDomain)));
        Spanned messagePart =
                AndroidUtils.formatHTML(
                        getString(
                                R.string.wallet_sign_message_details_message_section,
                                Html.escapeHtml(escapedMessage)));

        mSignMessageText.setText(TextUtils.concat(domainPart, messagePart));
    }
}
