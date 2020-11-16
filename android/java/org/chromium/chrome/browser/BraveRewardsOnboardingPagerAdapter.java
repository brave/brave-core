/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;
import androidx.viewpager.widget.PagerAdapter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.appcompat.widget.AppCompatImageView;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;

import java.util.Arrays;
import java.util.List;

public class BraveRewardsOnboardingPagerAdapter extends PagerAdapter {

    private static final List<String> mHeaders = Arrays.asList(
                ContextUtils.getApplicationContext().getResources().getString(R.string.welcome_to_brave_rewards),
                ContextUtils.getApplicationContext().getResources().getString(R.string.where_do_ads_show_up),
                ContextUtils.getApplicationContext().getResources().getString(R.string.when_do_you_receive_rewards),
                ContextUtils.getApplicationContext().getResources().getString(R.string.giving_back_made_effortless),
                ContextUtils.getApplicationContext().getResources().getString(R.string.say_thank_you_with_tips),
                ContextUtils.getApplicationContext().getResources().getString(R.string.what_can_you_do_with_tokens),
                ContextUtils.getApplicationContext().getResources().getString(R.string.you_are_done)
            );
    private static final List<String> mTexts = Arrays.asList(
                ContextUtils.getApplicationContext().getResources().getString(R.string.welcome_to_brave_rewards_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.where_do_ads_show_up_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.when_do_you_receive_rewards_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.giving_back_made_effortless_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.say_thank_you_with_tips_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.what_can_you_do_with_tokens_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.you_are_done_text)
            );
    private static final List<Integer> mImages = Arrays.asList(
                R.drawable.ic_onboarding_graphic_bat_ecosystem,
                R.drawable.ic_onboarding_graphic_android_brave_ads,
                R.drawable.ic_onboarding_graphic_bat_schedule,
                R.drawable.ic_onboarding_graphic_auto_contribute,
                R.drawable.ic_onboarding_graphic_tipping,
                R.drawable.ic_onboarding_graphic_cashback,
                R.drawable.ic_onboarding_graphic_completed
            );

    @Override
    public Object instantiateItem(ViewGroup collection, int position) {
        View view = LayoutInflater.from(ContextUtils.getApplicationContext()).inflate(R.layout.brave_rewards_onboarding_item_layout, null);
        TextView titleView = view.findViewById(R.id.title_view);
        titleView.setText(mHeaders.get(position));
        TextView textView = view.findViewById(R.id.text_view);
        textView.setText(mTexts.get(position));
        AppCompatImageView imageView = view.findViewById(R.id.image_view);
        imageView.setImageResource(mImages.get(position));
        collection.addView(view);
        return view;
    }

    @Override
    public void destroyItem(ViewGroup collection, int position, Object view) {
        collection.removeView((View) view);
    }

    @Override
    public int getCount() {
        return mHeaders.size();
    }

    @Override
    public boolean isViewFromObject(View view, Object object) {
        return view == object;
    }
}
