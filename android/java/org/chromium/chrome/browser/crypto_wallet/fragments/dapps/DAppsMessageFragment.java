/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.os.Bundle;
import android.text.Html;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.brave_wallet.mojom.SignDataUnion;
import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Validations;

/** Fragment to show DApps-related messages */
public class DAppsMessageFragment extends BaseDAppsFragment {
    private static final String TAG = "DAppsMessageFragment";
    private SignMessageRequest mCurrentSignMessageRequest;
    private boolean mUnicodeEscapeVersion;
    private TextView mSignMessageText;

    public DAppsMessageFragment(SignMessageRequest currentSignMessageRequest) {
        mCurrentSignMessageRequest = currentSignMessageRequest;
        assert mCurrentSignMessageRequest != null;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_dapps_message, container, false);
        mSignMessageText = view.findViewById(R.id.sign_message_text);

        mUnicodeEscapeVersion = false;
        final String message;
        final boolean isEip712;
        if (mCurrentSignMessageRequest.signData.which() == SignDataUnion.Tag.EthStandardSignData) {
            message = mCurrentSignMessageRequest.signData.getEthStandardSignData().message;
            isEip712 = false;
        } else if (mCurrentSignMessageRequest.signData.which()
                == SignDataUnion.Tag.EthSignTypedData) {
            message = mCurrentSignMessageRequest.signData.getEthSignTypedData().message;
            isEip712 = true;
        } else if (mCurrentSignMessageRequest.signData.which()
                == SignDataUnion.Tag.SolanaSignData) {
            message = mCurrentSignMessageRequest.signData.getSolanaSignData().message;
            isEip712 = false;
        } else if (mCurrentSignMessageRequest.signData.which() == SignDataUnion.Tag.EthSiweData) {
            message = mCurrentSignMessageRequest.signData.getEthSiweData().statement;
            isEip712 = false;
        } else {
            message = "";
            isEip712 = false;
        }
        updateTextEthSign(mUnicodeEscapeVersion, message, isEip712);
        if (Validations.hasUnicode(message)) {
            mSignMessageText.setLines(12);
            view.findViewById(R.id.non_ascii_warning_layout).setVisibility(View.VISIBLE);
            TextView warningLinkText = view.findViewById(R.id.non_ascii_warning_text_link);
            warningLinkText.setOnClickListener(
                    v -> {
                        warningLinkText.setText(
                                mUnicodeEscapeVersion
                                        ? getString(R.string.wallet_non_ascii_characters_original)
                                        : getString(R.string.wallet_non_ascii_characters_ascii));
                        mUnicodeEscapeVersion = !mUnicodeEscapeVersion;
                        updateTextEthSign(mUnicodeEscapeVersion, message, isEip712);
                    });
        }

        return view;
    }

    private void updateTextEthSign(
            boolean unicodeEscape, final String message, final boolean isEip712) {
        String escapedDomain = "";
        if (isEip712) {
            String domain = mCurrentSignMessageRequest.signData.getEthSignTypedData().domain;
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
