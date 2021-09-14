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
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.chrome.R;

public class AccountSpinnerAdapter extends BaseAdapter {
    Context context;
    int[] pictures;
    String[] accountNames;
    String[] accountTitles;
    LayoutInflater inflater;

    public AccountSpinnerAdapter(Context applicationContext, int[] pictures, String[] accountNames,
            String[] accountTitles) {
        this.context = applicationContext;
        this.pictures = pictures;
        this.accountNames = accountNames;
        this.accountTitles = accountTitles;
        inflater = (LayoutInflater.from(applicationContext));
    }

    public String getNameAtPosition(int position) {
        if (position < accountNames.length) {
            return accountNames[position];
        }

        return "";
    }

    public String getTitleAtPosition(int position) {
        if (position < accountTitles.length) {
            return accountTitles[position];
        }

        return "";
    }

    @Override
    public int getCount() {
        return accountNames.length;
    }

    @Override
    public Object getItem(int i) {
        return null;
    }

    @Override
    public long getItemId(int i) {
        return 0;
    }

    @Override
    public View getView(int i, View view, ViewGroup viewGroup) {
        view = inflater.inflate(R.layout.account_spinner_items, null);
        ImageView icon = (ImageView) view.findViewById(R.id.account_picture);
        TextView name = (TextView) view.findViewById(R.id.account_name_text);
        TextView value = (TextView) view.findViewById(R.id.account_value_text);
        icon.setImageResource(pictures[i]);
        name.setText(accountNames[i]);
        value.setText(stripTitle(accountTitles[i]));

        return view;
    }

    private String stripTitle(String title) {
        String newTitle = "";

        if (title.length() > 6) {
            newTitle = title.substring(0, 6) + "***" + title.substring(title.length() - 5);
        }

        return newTitle;
    }
}