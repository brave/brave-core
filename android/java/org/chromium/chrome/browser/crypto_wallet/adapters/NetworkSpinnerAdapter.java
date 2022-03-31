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
import android.widget.SpinnerAdapter;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class NetworkSpinnerAdapter extends BaseAdapter implements SpinnerAdapter {
    private Context context;
    private String[] networkNames;
    private String[] networkShortNames;
    private LayoutInflater inflater;
    private ExecutorService mExecutor;
    private Handler mHandler;

    public NetworkSpinnerAdapter(
            Context applicationContext, String[] networkNames, String[] networkShortNames) {
        assert networkNames.length == networkShortNames.length;
        this.context = applicationContext;
        this.networkNames = networkNames;
        this.networkShortNames = networkShortNames;
        inflater = (LayoutInflater.from(applicationContext));
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    public String getNameAtPosition(int position) {
        if (position < networkNames.length) {
            return networkNames[position];
        }

        return "";
    }

    public String getShortNameAtPosition(int position) {
        if (position < networkShortNames.length) {
            return networkShortNames[position];
        }

        return "";
    }

    @Override
    public int getCount() {
        return networkNames.length;
    }

    @Override
    public Object getItem(int i) {
        return getNameAtPosition(i);
    }

    @Override
    public long getItemId(int i) {
        return 0;
    }

    @Override
    // TODO(samartnik): this one requires review, for now just suppress to force lint checks.
    // Warning: Unconditional layout inflation from view adapter: Should use View Holder pattern
    // (use recycled view passed into this method as the second parameter) for smoother scrolling
    // [ViewHolder]
    @SuppressLint("ViewHolder")
    public View getView(int i, View view, ViewGroup viewGroup) {
        view = inflater.inflate(R.layout.network_spinner_items, null);
        TextView name = (TextView) view.findViewById(R.id.network_name_text);
        name.setText(networkShortNames[i]);
        ImageView networkPicture = view.findViewById(R.id.network_picture);
        networkPicture.setVisibility(View.GONE);
        name.setCompoundDrawablesRelativeWithIntrinsicBounds(null, null, null, null);

        return view;
    }

    @Override
    public View getDropDownView(int i, View view, ViewGroup viewGroup) {
        view = inflater.inflate(R.layout.network_spinner_items, null);
        TextView name = (TextView) view.findViewById(R.id.network_name_text);
        name.setText(networkNames[i]);
        ImageView networkPicture = view.findViewById(R.id.network_picture);
        Utils.setBlockiesBitmapResource(
                mExecutor, mHandler, networkPicture, networkNames[i], false);

        return view;
    }
}
