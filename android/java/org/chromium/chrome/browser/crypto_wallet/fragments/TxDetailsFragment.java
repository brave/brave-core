/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;

public class TxDetailsFragment extends Fragment {
    private TransactionInfo mTxInfo;

    public static TxDetailsFragment newInstance(TransactionInfo txInfo) {
        return new TxDetailsFragment(txInfo);
    }

    private TxDetailsFragment(TransactionInfo txInfo) {
        mTxInfo = txInfo;
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_transaction_details, container, false);

        return view;
    }
}
