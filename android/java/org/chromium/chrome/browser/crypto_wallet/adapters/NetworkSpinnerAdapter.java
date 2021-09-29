/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.SpinnerAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

public class NetworkSpinnerAdapter extends BaseAdapter implements SpinnerAdapter {
    Context context;
    String[] networkNames;
    String[] networkShortNames;
    LayoutInflater inflater;

    public NetworkSpinnerAdapter(Context applicationContext, String[] networkNames,
            String[] networkShortNames) {
        assert networkNames.length == networkShortNames.length;
        this.context = applicationContext;
        this.networkNames = networkNames;
        this.networkShortNames = networkShortNames;
        inflater = (LayoutInflater.from(applicationContext));
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
    public View getView(int i, View view, ViewGroup viewGroup) {
        view = inflater.inflate(R.layout.network_spinner_items, null);
        TextView name = (TextView) view.findViewById(R.id.network_name_text);
        name.setText(networkShortNames[i]);

        return view;
    }

    @Override
    public View getDropDownView(int i, View view, ViewGroup viewGroup) {
        view = inflater.inflate(R.layout.network_spinner_items, null);
        TextView name = (TextView) view.findViewById(R.id.network_name_text);
        name.setText(networkNames[i]);

        return view;
    }
}
