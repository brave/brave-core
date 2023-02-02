/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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

import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.Validations;

public class DAppsMessageFragment extends BaseDAppsFragment {
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
        updateText(mUnicodeEscapeVersion);
        if (Validations.hasUnicode(mCurrentSignMessageRequest.message)) {
            mSignMessageText.setLines(12);
            view.findViewById(R.id.non_ascii_warning_layout).setVisibility(View.VISIBLE);
            TextView warningLinkText = view.findViewById(R.id.non_ascii_warning_text_link);
            warningLinkText.setOnClickListener(v -> {
                warningLinkText.setText(mUnicodeEscapeVersion
                                ? getString(R.string.wallet_non_ascii_characters_original)
                                : getString(R.string.wallet_non_ascii_characters_ascii));
                mUnicodeEscapeVersion = !mUnicodeEscapeVersion;
                updateText(mUnicodeEscapeVersion);
            });
        }

        return view;
    }

    private void updateText(boolean unicodeEscape) {
        String escapedDomain = unicodeEscape
                ? Validations.unicodeEscape(mCurrentSignMessageRequest.domain)
                : mCurrentSignMessageRequest.domain;
        String escapedMessage = unicodeEscape
                ? Validations.unicodeEscape(mCurrentSignMessageRequest.message)
                : mCurrentSignMessageRequest.message;

        Spanned domainPart = escapedDomain.isEmpty()
                ? new SpannableString("")
                : AndroidUtils.formatHTML(
                        getString(R.string.wallet_sign_message_details_domain_section,
                                Html.escapeHtml(escapedDomain)));
        Spanned messagePart = AndroidUtils.formatHTML(
                getString(R.string.wallet_sign_message_details_message_section,
                        Html.escapeHtml(escapedMessage)));

        mSignMessageText.setText(TextUtils.concat(domainPart, messagePart));
    }
}
