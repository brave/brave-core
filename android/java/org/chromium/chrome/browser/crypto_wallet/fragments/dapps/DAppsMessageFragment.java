/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.os.Bundle;
import android.text.SpannableStringBuilder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.Validations;

public class DAppsMessageFragment extends BaseDAppsFragment {
    private SignMessageRequest mCurrentSignMessageRequest;
    private boolean mUnicodeEscapeVersion;

    public DAppsMessageFragment(SignMessageRequest currentSignMessageRequest) {
        mCurrentSignMessageRequest = currentSignMessageRequest;
        assert mCurrentSignMessageRequest != null;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_dapps_message, container, false);
        TextView signMessageText = view.findViewById(R.id.sign_message_text);
        signMessageText.setText(mCurrentSignMessageRequest.message);
        mUnicodeEscapeVersion = false;
        if (Validations.hasUnicode(mCurrentSignMessageRequest.message)) {
            view.findViewById(R.id.non_ascii_warning_layout).setVisibility(View.VISIBLE);
            TextView warningLinkText = view.findViewById(R.id.non_ascii_warning_text_link);
            warningLinkText.setOnClickListener(v -> {
                if (!mUnicodeEscapeVersion) {
                    warningLinkText.setText(getString(R.string.wallet_non_ascii_characters_ascii));
                    signMessageText.setText(
                            Validations.unicodeEscape(mCurrentSignMessageRequest.message));
                } else {
                    warningLinkText.setText(
                            getString(R.string.wallet_non_ascii_characters_original));
                    signMessageText.setText(mCurrentSignMessageRequest.message);
                }
                mUnicodeEscapeVersion = !mUnicodeEscapeVersion;
            });
        }

        return view;
    }
}
