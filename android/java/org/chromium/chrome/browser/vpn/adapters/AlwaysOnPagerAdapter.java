/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.adapters;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.PagerAdapter;

import org.chromium.chrome.R;

import java.util.Arrays;
import java.util.List;

public class AlwaysOnPagerAdapter extends PagerAdapter {
    private Context mContext;
    private List<Integer> mImageResources =
            Arrays.asList(
                    R.drawable.ic_vpn_always_on_1,
                    R.drawable.ic_vpn_always_on_2,
                    R.drawable.ic_vpn_always_on_3);
    private List<Integer> mTexts =
            Arrays.asList(
                    R.string.kill_switch_tutorial_text_1,
                    R.string.kill_switch_tutorial_text_2,
                    R.string.kill_switch_tutorial_text_3);

    public AlwaysOnPagerAdapter(Context context) {
        this.mContext = context;
    }

    @NonNull
    @Override
    public Object instantiateItem(ViewGroup collection, int position) {
        @SuppressLint("InflateParams")
        View view =
                LayoutInflater.from(mContext)
                        .inflate(R.layout.kill_switch_tutorial_item_layout, null);
        TextView killSwitchTutorialText = view.findViewById(R.id.kill_switch_tutorial_text);
        killSwitchTutorialText.setText(mContext.getResources().getString(mTexts.get(position)));
        ImageView killSwitchTutorialImage = view.findViewById(R.id.kill_switch_tutorial_image);
        killSwitchTutorialImage.setImageResource(mImageResources.get(position));
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
