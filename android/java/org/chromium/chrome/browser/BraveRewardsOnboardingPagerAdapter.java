/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.appcompat.widget.AppCompatImageView;
import androidx.viewpager.widget.PagerAdapter;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.profiles.Profile;

import java.util.Arrays;
import java.util.List;

public class BraveRewardsOnboardingPagerAdapter extends PagerAdapter {

    private static final int FEWER_OPTIONS = 7;
    private static final int MORE_OPTIONS = 8;
    private boolean shouldShowMoreOptions;

    private static final List<String> mHeaders = Arrays.asList(
                ContextUtils.getApplicationContext().getResources().getString(R.string.welcome_to_brave_rewards),
                ContextUtils.getApplicationContext().getResources().getString(R.string.where_do_ads_show_up),
                ContextUtils.getApplicationContext().getResources().getString(R.string.when_do_you_receive_rewards),
                ContextUtils.getApplicationContext().getResources().getString(R.string.giving_back_made_effortless),
                ContextUtils.getApplicationContext().getResources().getString(R.string.say_thank_you_with_tips),
                ContextUtils.getApplicationContext().getResources().getString(R.string.what_can_you_do_with_tokens),
                ContextUtils.getApplicationContext().getResources().getString(R.string.you_are_done),
                ContextUtils.getApplicationContext().getResources().getString(R.string.you_are_done)
            );
    private static final List<String> mTexts = Arrays.asList(
                ContextUtils.getApplicationContext().getResources().getString(R.string.welcome_to_brave_rewards_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.where_do_ads_show_up_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.when_do_you_receive_rewards_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.giving_back_made_effortless_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.say_thank_you_with_tips_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.what_can_you_do_with_tokens_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.you_are_done_text),
                ContextUtils.getApplicationContext().getResources().getString(R.string.you_are_done_text)
            );
    private static final List<Integer> mImages = Arrays.asList(
                R.drawable.ic_onboarding_graphic_bat_ecosystem,
                R.drawable.ic_onboarding_graphic_android_brave_ads,
                R.drawable.ic_onboarding_graphic_bat_schedule,
                R.drawable.ic_onboarding_graphic_auto_contribute,
                R.drawable.ic_onboarding_graphic_tipping,
                R.drawable.ic_onboarding_graphic_cashback,
                R.drawable.ic_onboarding_graphic_completed,
                R.drawable.ic_onboarding_graphic_completed
            );

    public void setOnboardingType(boolean shouldShowMoreOptions) {
        this.shouldShowMoreOptions = shouldShowMoreOptions;
    }

    @Override
    public Object instantiateItem(ViewGroup collection, int position) {
        View view;
        if (shouldShowMoreOptions
            && position == (MORE_OPTIONS-2)) {
            view = LayoutInflater.from(ContextUtils.getApplicationContext()).inflate(R.layout.brave_rewards_onboarding_ac_layout, null);
            RadioGroup hourRadioGroup = view.findViewById(R.id.hour_radio_group);
            int adsPerHour = BraveRewardsNativeWorker.getInstance().GetAdsPerHour();
            RadioButton defaultRadioButton;
            switch(adsPerHour) {
                case 1:
                    defaultRadioButton = ((RadioButton) view.findViewById(R.id.hour_1_radio));
                    break;
                case 2:
                    defaultRadioButton = ((RadioButton) view.findViewById(R.id.hour_2_radio));
                    break;
                case 3:
                    defaultRadioButton = ((RadioButton) view.findViewById(R.id.hour_3_radio));
                    break;
                case 4:
                    defaultRadioButton = ((RadioButton) view.findViewById(R.id.hour_4_radio));
                    break;
                case 5:
                    defaultRadioButton = ((RadioButton) view.findViewById(R.id.hour_5_radio));
                    break;
                default:
                    defaultRadioButton = ((RadioButton) view.findViewById(R.id.hour_1_radio));
                    break;
            }
            defaultRadioButton.setChecked(true);
            hourRadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                    int hour = adsPerHour;
                    if (checkedId == R.id.hour_1_radio) {
                        hour = 1;
                    } else if (checkedId == R.id.hour_2_radio) {
                        hour = 2;
                    } else if (checkedId == R.id.hour_3_radio) {
                        hour = 3;
                    } else if (checkedId == R.id.hour_4_radio) {
                        hour = 4;
                    } else if (checkedId == R.id.hour_5_radio) {
                        hour = 5;
                    }
                    if (BraveAdsNativeHelper.nativeIsBraveAdsEnabled(
                                Profile.getLastUsedRegularProfile())) {
                        BraveRewardsNativeWorker.getInstance().SetAdsPerHour(hour);
                    }
                }
            });

            RadioGroup contributeRadioGroup = view.findViewById(R.id.contribute_radio_group);
            BraveRewardsNativeWorker.getInstance().SetAutoContributionAmount(5);
            ((RadioButton) view.findViewById(R.id.contribute_5_radio)).setChecked(true);
            contributeRadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                    double contribute = 15;
                    if (checkedId == R.id.contribute_5_radio) {
                        contribute = 5;
                    } else if (checkedId == R.id.contribute_15_radio) {
                        contribute = 15;
                    } else if (checkedId == R.id.contribute_25_radio) {
                        contribute = 25;
                    } else if (checkedId == R.id.contribute_50_radio) {
                        contribute = 50;
                    }
                    BraveRewardsNativeWorker.getInstance().SetAutoContributionAmount(contribute);
                }
            });
        } else {
            view = LayoutInflater.from(ContextUtils.getApplicationContext()).inflate(R.layout.brave_rewards_onboarding_item_layout, null);
            TextView titleView = view.findViewById(R.id.title_view);
            titleView.setText(mHeaders.get(position));
            TextView textView = view.findViewById(R.id.text_view);
            textView.setText(mTexts.get(position));
            AppCompatImageView imageView = view.findViewById(R.id.image_view);
            imageView.setImageResource(mImages.get(position));
        }
        collection.addView(view);
        return view;
    }

    @Override
    public void destroyItem(ViewGroup collection, int position, Object view) {
        collection.removeView((View) view);
    }

    @Override
    public int getCount() {
        return shouldShowMoreOptions ? MORE_OPTIONS : FEWER_OPTIONS;
    }

    @Override
    public boolean isViewFromObject(View view, Object object) {
        return view == object;
    }
}
