/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.content.Context;
import android.support.v4.view.PagerAdapter;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Button;
import android.view.LayoutInflater;

import org.chromium.chrome.R;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.ntp_background_images.util.NewTabPageListener;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;

import java.util.List;
import java.util.Arrays;

public class OnboardingBottomSheetViewPagerAdapter extends PagerAdapter {
    private int mOnboardingType;
    private NewTabPageListener mNewTabPageListener;

    private static final Context mContext = ContextUtils.getApplicationContext();
    private static final List<String> mHeaders = Arrays.asList(
                mContext.getResources().getString(R.string.privacy_protection),
                mContext.getResources().getString(R.string.save_data_and_battery),
                mContext.getResources().getString(R.string.websites_load_faster),
                mContext.getResources().getString(R.string.get_weekly_updates)
            );
    private static final List<String> mTexts = Arrays.asList(
                mContext.getResources().getString(R.string.privacy_protection_text),
                mContext.getResources().getString(R.string.save_data_and_battery_text),
                mContext.getResources().getString(R.string.websites_load_faster_text),
                mContext.getResources().getString(R.string.get_weekly_updates_text)
            );

    public OnboardingBottomSheetViewPagerAdapter(int onboardingType, NewTabPageListener newTabPageListener) {
        this.mOnboardingType = onboardingType;
        this.mNewTabPageListener = newTabPageListener;
    }

    @Override
    public int getCount() {
        if (mOnboardingType != OnboardingPrefManager.ONBOARDING_INVALID_OPTION) {
            return 1;
        }
        return mHeaders.size();
    }

    @Override
    public boolean isViewFromObject(View view, Object viewObject) {
        return viewObject == view;
    }

    @Override
    public Object instantiateItem(final ViewGroup container, int position) {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        ViewGroup layout = (ViewGroup) inflater.inflate(R.layout.onboarding_pager_layout,
                           container, false);

        switch (mOnboardingType) {
        case OnboardingPrefManager.ONBOARDING_ADS:
            position = 0;
            break;
        case OnboardingPrefManager.ONBOARDING_DATA_SAVED:
            position = 1;
            break;
        case OnboardingPrefManager.ONBOARDING_TIME:
            position = 2;
            break;
        }

        TextView mHeader = layout.findViewById(R.id.onboarding_header);
        mHeader.setText(mHeaders.get(position));

        TextView mText = layout.findViewById(R.id.onboarding_text);
        mText.setText(mTexts.get(position));

        Button mAction = layout.findViewById(R.id.btn_turn_on_privacy_stats);
        mAction.setText(mContext.getResources().getString(R.string.turn_on_privacy_stats));
        mAction.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

            }
        });

        container.addView(layout);
        return layout;
    }

    @Override
    public void destroyItem(ViewGroup container, int position, Object object) {
        container.removeView((View)object);
    }
}
