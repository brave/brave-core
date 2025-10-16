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
import android.widget.TextView;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.SignDataUnion;
import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.Validations;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Fragment used by DApps sign operation */
@NullMarked
public class SignMessageFragment extends BaseDAppsBottomSheetDialogFragment {
    private static final String TAG = "SignMessageFragment";

    private SignMessageRequest mCurrentSignMessageRequest;
    private boolean mUnicodeEscapeVersion;
    private TextView mSignMessageText;
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
        mSignMessageText = view.findViewById(R.id.sign_message_text);
        mAccountImage = view.findViewById(R.id.fragment_sign_msg_cv_iv_account);
        mAccountName = view.findViewById(R.id.fragment_sign_msg_tv_account_name);
        mNetworkName = view.findViewById(R.id.fragment_sign_msg_tv_network_name);

        mBtCancel = view.findViewById(R.id.fragment_sign_msg_btn_cancel);
        mBtSign = view.findViewById(R.id.fragment_sign_msg_btn_sign);
        mWebSite = view.findViewById(R.id.domain);
        initComponents();

        return view;
    }

    private void notifySignMessageRequestProcessed(final boolean approved) {
        getBraveWalletService()
                .notifySignMessageRequestProcessed(
                        approved, mCurrentSignMessageRequest.id, null, null);
        fillSignMessageInfo(false);
    }

    private void initComponents() {
        fillSignMessageInfo(true);
    }

    private void fillSignMessageInfo(final boolean init) {
        getBraveWalletService()
                .getPendingSignMessageRequests(
                        requests -> {
                            maybeHandlePendingRequests(init, requests);
                        });
    }

    private void maybeHandlePendingRequests(final boolean init,
                                            @Nullable SignMessageRequest[] requests) {
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
        if (mCurrentSignMessageRequest.signData.which()
                == SignDataUnion.Tag.EthStandardSignData) {
            message =
                    mCurrentSignMessageRequest.signData
                            .getEthStandardSignData()
                            .message;
            isEip712 = false;
        } else if (mCurrentSignMessageRequest.signData.which()
                == SignDataUnion.Tag.EthSignTypedData) {
            message =
                    mCurrentSignMessageRequest.signData
                            .getEthSignTypedData()
                            .messageJson;
            isEip712 = true;
        } else if (mCurrentSignMessageRequest.signData.which()
                == SignDataUnion.Tag.SolanaSignData) {
            message =
                    mCurrentSignMessageRequest.signData
                            .getSolanaSignData()
                            .message;
            isEip712 = false;
        } else if (mCurrentSignMessageRequest.signData.which()
                == SignDataUnion.Tag.EthSiweData) {
            message =
                    mCurrentSignMessageRequest.signData
                            .getEthSiweData()
                            .statement;
            isEip712 = false;
        } else {
            message = "";
            isEip712 = false;
        }

        updateTextEthSign(mUnicodeEscapeVersion, message, isEip712);

        if (Validations.hasUnicode(message)) {
            mSignMessageText.setLines(12);
            View view = getView();
            if (view != null) {
                view.findViewById(R.id.non_ascii_warning_layout)
                        .setVisibility(View.VISIBLE);
                TextView warningLinkText =
                        view.findViewById(R.id.non_ascii_warning_text_link);
                warningLinkText.setOnClickListener(
                        v -> {
                            warningLinkText.setText(
                                    mUnicodeEscapeVersion
                                            ? getString(
                                            R.string
                                                    .wallet_non_ascii_characters_original)
                                            : getString(
                                            R.string
                                                    .wallet_non_ascii_characters_ascii));
                            mUnicodeEscapeVersion = !mUnicodeEscapeVersion;
                            updateTextEthSign(
                                    mUnicodeEscapeVersion, message, isEip712);
                        });
            }
        }

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

    private void updateTextEthSign(
            final boolean unicodeEscape, final String message, final boolean isEip712) {
        String escapedDomain = "";
        if (isEip712) {
            String domain = mCurrentSignMessageRequest.signData.getEthSignTypedData().domainJson;
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
