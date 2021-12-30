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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

public class BraveRewardsOnboardingPagerAdapter extends PagerAdapter {
    private boolean shouldShowMoreOptions;

    public void setOnboardingType(boolean shouldShowMoreOptions) {
        this.shouldShowMoreOptions = shouldShowMoreOptions;
    }

    @Override
    public Object instantiateItem(ViewGroup collection, int position) {
        View view;
        Context context = ContextUtils.getApplicationContext();
        if (shouldShowMoreOptions && position == (getImages().size() - 2)) {
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
            BraveRewardsNativeWorker.getInstance().SetAutoContributionAmount(1);
            ((RadioButton) view.findViewById(R.id.contribute_1_radio)).setChecked(true);
            contributeRadioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(RadioGroup radioGroup, int checkedId) {
                    double contribute = 1;
                    if (checkedId == R.id.contribute_1_radio) {
                        contribute = 1;
                    } else if (checkedId == R.id.contribute_2_radio) {
                        contribute = 2;
                    } else if (checkedId == R.id.contribute_3_radio) {
                        contribute = 3;
                    } else if (checkedId == R.id.contribute_5_radio) {
                        contribute = 5;
                    }
                    BraveRewardsNativeWorker.getInstance().SetAutoContributionAmount(contribute);
                }
            });
            String countryCode = Locale.getDefault().getCountry();
            if (countryCode.equals("JP")) {
                view.findViewById(R.id.auto_contribute_layout).setVisibility(View.GONE);
            }
        } else {
            view = LayoutInflater.from(ContextUtils.getApplicationContext()).inflate(R.layout.brave_rewards_onboarding_item_layout, null);
            TextView titleView = view.findViewById(R.id.title_view);
            titleView.setText(getTitles(context).get(position));
            TextView textView = view.findViewById(R.id.text_view);
            textView.setText(getTexts(context).get(position));
            AppCompatImageView imageView = view.findViewById(R.id.image_view);
            imageView.setImageResource(getImages().get(position));
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
        return shouldShowMoreOptions ? getImages().size() : getImages().size() - 1;
    }

    @Override
    public boolean isViewFromObject(View view, Object object) {
        return view == object;
    }

    private List<String> getTitles(Context context) {
        List<String> headers = new ArrayList();
        headers.add(context.getResources().getString(R.string.welcome_to_brave_rewards));
        headers.add(context.getResources().getString(R.string.where_do_ads_show_up));
        headers.add(context.getResources().getString(R.string.when_do_you_receive_rewards));
        String countryCode = Locale.getDefault().getCountry();
        if (!countryCode.equals("JP")) {
            headers.add(context.getResources().getString(R.string.giving_back_made_effortless));
        }
        headers.add(context.getResources().getString(R.string.say_thank_you_with_tips));
        headers.add(context.getResources().getString(R.string.what_can_you_do_with_tokens));
        headers.add(context.getResources().getString(R.string.you_are_done));
        headers.add(context.getResources().getString(R.string.you_are_done));
        return headers;
    }

    private List<String> getTexts(Context context) {
        List<String> texts = new ArrayList();
        texts.add(context.getResources().getString(R.string.welcome_to_brave_rewards_text));
        texts.add(context.getResources().getString(R.string.where_do_ads_show_up_text));
        texts.add(context.getResources().getString(R.string.when_do_you_receive_rewards_text));
        String countryCode = Locale.getDefault().getCountry();
        if (!countryCode.equals("JP")) {
            texts.add(context.getResources().getString(R.string.giving_back_made_effortless_text));
        }
        texts.add(context.getResources().getString(R.string.say_thank_you_with_tips_text));
        texts.add(context.getResources().getString(R.string.what_can_you_do_with_tokens_text));
        texts.add(context.getResources().getString(R.string.you_are_done_text));
        texts.add(context.getResources().getString(R.string.you_are_done_text));
        return texts;
    }

    private List<Integer> getImages() {
        List<Integer> images = new ArrayList();
        images.add(R.drawable.ic_onboarding_graphic_bat_ecosystem);
        images.add(R.drawable.ic_onboarding_graphic_android_brave_ads);
        images.add(R.drawable.ic_onboarding_graphic_bat_schedule);
        String countryCode = Locale.getDefault().getCountry();
        if (!countryCode.equals("JP")) {
            images.add(R.drawable.ic_onboarding_graphic_auto_contribute);
        }
        images.add(R.drawable.ic_onboarding_graphic_tipping);
        images.add(R.drawable.ic_onboarding_graphic_cashback);
        images.add(R.drawable.ic_onboarding_graphic_completed);
        images.add(R.drawable.ic_onboarding_graphic_completed);
        return images;
    }
}
