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

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.SignMessageError;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;

/** Fragment used by DApps for sign-in error message */
public class SignMessageErrorFragment extends BaseDAppsBottomSheetDialogFragment {
    private static final String TAG = "SMEF";

    private WalletModel mWalletModel;
    private SignMessageError mCurrentSignMessageError;
    private Button mBtClose;
    private TextView mTextViewUrl;
    private TextView mTextViewHost;
    private TextView mTextViewReason;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
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
        View view = inflater.inflate(R.layout.fragment_sign_message_error, container, false);

        mBtClose = view.findViewById(R.id.fragment_sign_msg_err_btn_close);
        mTextViewUrl = view.findViewById(R.id.fragment_sign_msg_err_tv_url);
        mTextViewHost = view.findViewById(R.id.fragment_sign_msg_err_tv_host);
        mTextViewReason = view.findViewById(R.id.fragment_sign_msg_err_tv_reason);

        initComponents();

        return view;
    }

    private void initComponents() {
        fillSignMessageErrorInfo(true);
    }

    private void fillSignMessageErrorInfo(boolean init) {
        getBraveWalletService()
                .getPendingSignMessageErrors(
                        errors -> {
                            if (errors == null || errors.length == 0) {
                                Intent intent = new Intent();
                                getActivity().setResult(Activity.RESULT_OK, intent);
                                getActivity().finish();
                                return;
                            }

                            mCurrentSignMessageError = errors[0];

                            mTextViewUrl.setText(mCurrentSignMessageError.originInfo.originSpec);
                            mTextViewHost.setText(mCurrentSignMessageError.originInfo.eTldPlusOne);
                            mTextViewReason.setText(mCurrentSignMessageError.localizedErrMsg);

                            if (init) {
                                mBtClose.setOnClickListener(
                                        v -> {
                                            notifySignMessageErrorProcessed();
                                        });
                            }
                        });
    }

    private void notifySignMessageErrorProcessed() {
        getBraveWalletService().notifySignMessageErrorProcessed(mCurrentSignMessageError.id);
        fillSignMessageErrorInfo(false);
    }
}
