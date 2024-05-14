/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.appcompat.widget.AppCompatImageView;
import androidx.viewpager.widget.PagerAdapter;

import com.google.android.material.slider.LabelFormatter;
import com.google.android.material.slider.Slider;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.profiles.ProfileManager;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class BraveRewardsOnboardingPagerAdapter extends PagerAdapter {
    private static final String TAG = "RewardsOnboarding";
    private boolean shouldShowMoreOptions;
    private TextView adsPerHourText;
    private static final String JAPAN_COUNTRY_CODE = "JP";
    private String countryCode = BraveRewardsNativeWorker.getInstance().getCountryCode();
    private List<Integer> adsPerHourValues = Arrays.asList(0, 1, 2, 3, 4, 5, 10);

    public void setOnboardingType(boolean shouldShowMoreOptions) {
        this.shouldShowMoreOptions = shouldShowMoreOptions;
    }

    @Override
    public Object instantiateItem(ViewGroup collection, int position) {
        View view;
        Context context = ContextUtils.getApplicationContext();
        if (shouldShowMoreOptions && position == (getImages().size() - 2)) {
            int adsPerHour = BraveRewardsNativeWorker.getInstance().getAdsPerHour();
            int adsValue = adsPerHourValues.indexOf(adsPerHour);
            view = LayoutInflater.from(ContextUtils.getApplicationContext())
                           .inflate(R.layout.brave_rewards_onboarding_ac_layout, null);
            Slider adsPerHourSlider = view.findViewById(R.id.ads_per_hour_slider);
            adsPerHourSlider.setValueTo(adsPerHourValues.size() - 1);
            adsPerHourSlider.setValueFrom(0);
            adsPerHourSlider.setValue(adsValue);
            adsPerHourSlider.setLabelBehavior(LabelFormatter.LABEL_GONE);
            adsPerHourText = view.findViewById(R.id.ads_per_hour_text);
            adsPerHourText.setText(
                    String.format(ContextUtils.getApplicationContext().getResources().getString(
                                          R.string.ads_per_hour),
                            adsPerHour));
            adsPerHourSlider.addOnSliderTouchListener(touchListener);
        } else {
            view = LayoutInflater.from(ContextUtils.getApplicationContext()).inflate(R.layout.brave_rewards_onboarding_item_layout, null);
            TextView titleView = view.findViewById(R.id.title_view);
            titleView.setText(getTitles(context).get(position));
            TextView textView = view.findViewById(R.id.onboarding_text_view);
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
        if (!countryCode.equals(JAPAN_COUNTRY_CODE)) {
            headers.add(context.getResources().getString(R.string.giving_back_made_effortless));
        }
        headers.add(context.getResources().getString(R.string.say_thank_you_with_tips));
        headers.add(context.getResources().getString(R.string.you_are_done));
        headers.add(context.getResources().getString(R.string.you_are_done));
        return headers;
    }

    private List<String> getTexts(Context context) {
        List<String> texts = new ArrayList();
        texts.add(context.getResources().getString(R.string.welcome_to_brave_rewards_text));
        texts.add(context.getResources().getString(R.string.where_do_ads_show_up_text));
        if (!countryCode.equals(JAPAN_COUNTRY_CODE)) {
            texts.add(context.getResources().getString(R.string.giving_back_made_effortless_text));
        }
        texts.add(context.getResources().getString(R.string.say_thank_you_with_tips_text));
        texts.add(context.getResources().getString(R.string.you_are_done_text));
        texts.add(context.getResources().getString(R.string.you_are_done_text));
        return texts;
    }

    private List<Integer> getImages() {
        List<Integer> images = new ArrayList();
        images.add(R.drawable.ic_onboarding_graphic_bat_ecosystem);
        images.add(R.drawable.ic_onboarding_graphic_android_brave_ads);
        if (!countryCode.equals(JAPAN_COUNTRY_CODE)) {
            images.add(R.drawable.ic_onboarding_graphic_auto_contribute);
        }
        images.add(R.drawable.ic_onboarding_graphic_tipping);
        images.add(R.drawable.ic_onboarding_graphic_completed);
        images.add(R.drawable.ic_onboarding_graphic_completed);
        return images;
    }

    private final Slider.OnSliderTouchListener touchListener =
            new Slider.OnSliderTouchListener() {
                @Override
                public void onStartTrackingTouch(Slider slider) {
                    // Not implemented
                }

                @Override
                public void onStopTrackingTouch(Slider slider) {
                    try {
                        if (adsPerHourText != null) {
                            int adsValue = adsPerHourValues.get((int) slider.getValue());
                            adsPerHourText.setText(
                                    String.format(
                                            ContextUtils.getApplicationContext()
                                                    .getResources()
                                                    .getString(R.string.ads_per_hour),
                                            adsValue));
                        }
                        if (BraveAdsNativeHelper.nativeIsOptedInToNotificationAds(
                                ProfileManager.getLastUsedRegularProfile())) {
                            BraveRewardsNativeWorker.getInstance()
                                    .setAdsPerHour((int) slider.getValue());
                        }
                    } catch (Exception ex) {
                        Log.e(TAG, ex.getMessage());
                    }
                }
            };
}
