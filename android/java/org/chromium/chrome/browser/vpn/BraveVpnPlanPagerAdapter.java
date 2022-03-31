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
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.PagerAdapter;

import org.chromium.chrome.R;

public class BraveVpnPlanPagerAdapter extends PagerAdapter {
    private final Context context;
    public BraveVpnPlanPagerAdapter(Context context) {
        this.context = context;
    }

    @NonNull
    @Override
    public Object instantiateItem(ViewGroup collection, int position) {
        @SuppressLint("InflateParams")
        View view = LayoutInflater.from(context).inflate(R.layout.fragment_brave_vpn_plan, null);
        TextView planText1 = view.findViewById(R.id.plan_text_1);
        TextView planText2 = view.findViewById(R.id.plan_text_2);
        TextView planText3 = view.findViewById(R.id.plan_text_3);
        if (position == 0) {
            planText1.setText(context.getResources().getString(R.string.block_ads));
            planText2.setText(context.getResources().getString(R.string.across_all_apps));
            planText3.setText(context.getResources().getString(R.string.keep_your_ip_address));
        } else {
            planText1.setText(context.getResources().getString(R.string.supports_speed));
            planText2.setText(context.getResources().getString(R.string.use_ikev2));
            planText3.setText(context.getResources().getString(R.string.never_share_info));
        }
        collection.addView(view);
        return view;
    }

    @Override
    public void destroyItem(ViewGroup collection, int position, @NonNull Object view) {
        collection.removeView((View) view);
    }

    @Override
    public int getCount() {
        return 2;
    }

    @Override
    public boolean isViewFromObject(@NonNull View view, @NonNull Object object) {
        return view == object;
    }
}
