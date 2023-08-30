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

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.SignDataUnion;
import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Validations;
import org.chromium.url.internal.mojom.Origin;

import java.util.ArrayList;

/**
 * Fragment to show DApps-related messages
 */
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
            message = mCurrentSignMessageRequest.signData.getEthSiweData().statement; //??
            isEip712 = true;
        } else {
            message = "";
            isEip712 = false;
        }
        updateText(mUnicodeEscapeVersion, message, isEip712);
        if (Validations.hasUnicode(message)) {
            mSignMessageText.setLines(12);
            view.findViewById(R.id.non_ascii_warning_layout).setVisibility(View.VISIBLE);
            TextView warningLinkText = view.findViewById(R.id.non_ascii_warning_text_link);
            warningLinkText.setOnClickListener(v -> {
                warningLinkText.setText(mUnicodeEscapeVersion
                                ? getString(R.string.wallet_non_ascii_characters_original)
                                : getString(R.string.wallet_non_ascii_characters_ascii));
                mUnicodeEscapeVersion = !mUnicodeEscapeVersion;
                updateText(mUnicodeEscapeVersion, message, isEip712);
            });
        }

        return view;
    }

    private void updateText(boolean unicodeEscape, final String message, final boolean isEip712) {
        if (mCurrentSignMessageRequest.signData.which() == SignDataUnion.Tag.EthSiweData) {
            updateTextSiwe(unicodeEscape, message, isEip712);
        } else {
            updateTextEthSign(unicodeEscape, message, isEip712);
        }
    }

    private void updateTextEthSign(
            boolean unicodeEscape, final String message, final boolean isEip712) {
        String escapedDomain = "";
        if (isEip712) {
            String domain = mCurrentSignMessageRequest.signData.getEthSignTypedData().domain;
            escapedDomain = unicodeEscape ? Validations.unicodeEscape(domain) : domain;
        }
        String escapedMessage = unicodeEscape ? Validations.unicodeEscape(message) : message;

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

    private void updateTextSiwe(
            boolean unicodeEscape, final String message, final boolean isEip712) {
        assert mCurrentSignMessageRequest.signData.which() == SignDataUnion.Tag.EthSiweData;

        ArrayList<Spanned> allDetails = new ArrayList<>();
        addDetail(allDetails, R.string.wallet_siwe_message_details_origin_section,
                getOriginJson(mCurrentSignMessageRequest.signData.getEthSiweData().origin),
                unicodeEscape);
        addDetail(allDetails, R.string.wallet_siwe_message_details_address_section,
                mCurrentSignMessageRequest.signData.getEthSiweData().address, unicodeEscape);
        addDetail(allDetails, R.string.wallet_siwe_message_details_statement_section,
                mCurrentSignMessageRequest.signData.getEthSiweData().statement, unicodeEscape);
        addDetail(allDetails, R.string.wallet_siwe_message_details_uri_section,
                mCurrentSignMessageRequest.signData.getEthSiweData().uri.url, unicodeEscape);
        addDetail(allDetails, R.string.wallet_siwe_message_details_version_section,
                Integer.toString(mCurrentSignMessageRequest.signData.getEthSiweData().version),
                unicodeEscape);
        addDetail(allDetails, R.string.wallet_siwe_message_details_chain_id_section,
                Long.toString(mCurrentSignMessageRequest.signData.getEthSiweData().chainId),
                unicodeEscape);
        addDetail(allDetails, R.string.wallet_siwe_message_details_nonce_section,
                mCurrentSignMessageRequest.signData.getEthSiweData().nonce, unicodeEscape);
        addDetail(allDetails, R.string.wallet_siwe_message_details_issued_at_section,
                mCurrentSignMessageRequest.signData.getEthSiweData().issuedAt, unicodeEscape);
        addDetail(allDetails, R.string.wallet_siwe_message_details_expiration_time_section,
                mCurrentSignMessageRequest.signData.getEthSiweData().expirationTime, unicodeEscape);

        mSignMessageText.setText(TextUtils.concat(allDetails.toArray(new Spanned[0])));
    }

    private String escapeIfNeed(String detail, boolean unicodeEscape) {
        if (TextUtils.isEmpty(detail)) return null;
        if (unicodeEscape) return Validations.unicodeEscape(detail);
        return detail;
    }

    private void addDetail(
            ArrayList<Spanned> allDetails, int captionId, String value, boolean unicodeEscape) {
        if (TextUtils.isEmpty(value)) return;

        allDetails.add(AndroidUtils.formatHTML(
                getString(captionId, Html.escapeHtml(escapeIfNeed(value, unicodeEscape)))));
    }

    private String getOriginJson(Origin origin) {
        if (origin == null) return null;

        try {
            JSONObject jsonObject = new JSONObject();

            jsonObject.put("scheme", origin.scheme);
            jsonObject.put("host", origin.host);
            jsonObject.put("port", origin.port);

            return jsonObject.toString();
        } catch (JSONException e) {
            Log.e(TAG, e.getMessage());
        }

        return null;
    }
}
