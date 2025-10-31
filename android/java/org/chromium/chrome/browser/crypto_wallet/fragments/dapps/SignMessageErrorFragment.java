/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import org.chromium.brave_wallet.mojom.SignMessageError;
import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.fragments.WalletBottomSheetDialogFragment;

/** Fragment used by DApps for sign-in error message */
@NullMarked
public class SignMessageErrorFragment extends WalletBottomSheetDialogFragment {

    @MonotonicNonNull private SignMessageError mCurrentSignMessageError;
    private Button mBtClose;
    private TextView mTextViewUrl;
    private TextView mTextViewHost;
    private TextView mTextViewReason;

    @Override
    public View onCreateView(
            LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_sign_message_error, container, false);

        mBtClose = view.findViewById(R.id.fragment_sign_msg_err_btn_close);
        mTextViewUrl = view.findViewById(R.id.fragment_sign_msg_err_tv_url);
        mTextViewHost = view.findViewById(R.id.fragment_sign_msg_err_tv_host);
        mTextViewReason = view.findViewById(R.id.fragment_sign_msg_err_tv_reason);
        mBtClose.setOnClickListener(v -> notifySignMessageErrorProcessed());

        fillSignMessageErrorInfo();

        return view;
    }

    private void fillSignMessageErrorInfo() {
        getBraveWalletService()
                .getPendingSignMessageErrors(
                        errors -> {
                            if (errors == null || errors.length == 0) {
                                Intent intent = new Intent();
                                requireActivity().setResult(Activity.RESULT_OK, intent);
                                requireActivity().finish();
                                return;
                            }

                            mCurrentSignMessageError = errors[0];

                            mTextViewUrl.setText(mCurrentSignMessageError.originInfo.originSpec);
                            mTextViewHost.setText(mCurrentSignMessageError.originInfo.eTldPlusOne);
                            mTextViewReason.setText(mCurrentSignMessageError.localizedErrMsg);
                        });
    }

    private void notifySignMessageErrorProcessed() {
        if (mCurrentSignMessageError != null) {
            getBraveWalletService().notifySignMessageErrorProcessed(mCurrentSignMessageError.id);
            fillSignMessageErrorInfo();
        }
    }
}
