/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AccountSpinnerAdapter extends BaseAdapter {
    private Context context;
    private LayoutInflater inflater;
    private List<AccountInfo> mAccountInfos;
    private ExecutorService mExecutor;
    private Handler mHandler;

    public AccountSpinnerAdapter(Context applicationContext, List<AccountInfo> accountInfos) {
        this.context = applicationContext;
        inflater = (LayoutInflater.from(applicationContext));
        mAccountInfos = accountInfos;
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    public String getNameAtPosition(int position) {
        if (position < mAccountInfos.size()) {
            return mAccountInfos.get(position).name;
        }

        return "";
    }

    public String getAccountAddressAtPosition(int position) {
        if (position < mAccountInfos.size()) {
            return mAccountInfos.get(position).address;
        }

        return "";
    }

    @Override
    public int getCount() {
        return mAccountInfos.size();
    }

    @Override
    public Object getItem(int i) {
        return mAccountInfos.get(i);
    }

    @Override
    public long getItemId(int i) {
        return i;
    }

    @Override
    // TODO(samartnik): this one requires review, for now just suppress to force lint checks.
    // Warning: Unconditional layout inflation from view adapter: Should use View Holder pattern
    // (use recycled view passed into this method as the second parameter) for smoother scrolling
    // [ViewHolder]
    @SuppressLint("ViewHolder")
    public View getView(int i, View view, ViewGroup viewGroup) {
        view = inflater.inflate(R.layout.account_spinner_items, null);
        ImageView icon = view.findViewById(R.id.account_picture);
        TextView name = view.findViewById(R.id.account_name_text);
        TextView value = view.findViewById(R.id.account_value_text);
        AccountInfo accountInfo = mAccountInfos.get(i);
        Utils.setBlockiesBitmapResource(mExecutor, mHandler, icon, accountInfo.address, true);
        name.setText(accountInfo.name);
        value.setText(Utils.stripAccountAddress(accountInfo.address));

        return view;
    }

    public void setAccounts(List<AccountInfo> accounts) {
        mAccountInfos = accounts;
        notifyDataSetChanged();
    }

    public AccountInfo getSelectedAccountAt(int position) {
        return mAccountInfos.get(position);
    }
}
