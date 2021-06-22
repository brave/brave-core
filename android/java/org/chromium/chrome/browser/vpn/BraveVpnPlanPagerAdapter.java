/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.PagerAdapter;

import org.chromium.chrome.R;

public class BraveVpnPlanPagerAdapter extends PagerAdapter {
    private final Context context;
    public BraveVpnPlanPagerAdapter(Context context){
        this.context = context;
    }

    @NonNull
    @Override
    public Object instantiateItem(ViewGroup collection, int position) {
        @SuppressLint("InflateParams")
        View view = LayoutInflater.from(context).inflate(R.layout.fragment_brave_vpn_plan, null);
        collection.addView(view);
        return view;
    }

    @Override
    public void destroyItem(ViewGroup collection, int position, @NonNull Object view) {
        collection.removeView((View) view);
    }

    @Override
    public int getCount() {
        return 3;
    }

    @Override
    public boolean isViewFromObject(@NonNull View view, @NonNull Object object) {
        return view == object;
    }
}
