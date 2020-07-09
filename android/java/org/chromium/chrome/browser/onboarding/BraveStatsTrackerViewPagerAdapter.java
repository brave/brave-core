/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding;

import android.content.Context;
import androidx.viewpager.widget.PagerAdapter;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Button;
import android.widget.ImageView;
import android.view.LayoutInflater;
import com.airbnb.lottie.LottieAnimationView;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.LinearLayoutManager;

import org.chromium.chrome.R;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;

import java.util.List;
import java.util.Arrays;

public class BraveStatsTrackerViewPagerAdapter extends PagerAdapter {
    private static final Context mContext = ContextUtils.getApplicationContext();
    private static final List<String> mHeaders = Arrays.asList(
                mContext.getResources().getString(R.string.week),
                mContext.getResources().getString(R.string.month)
            );

    public BraveStatsTrackerViewPagerAdapter() {

    }

    @Override
    public int getCount() {
        return mHeaders.size();
    }

    @Override
    public CharSequence getPageTitle(int position) {
        // Generate title based on item position
        return mHeaders.get(position);
    }

    @Override
    public boolean isViewFromObject(View view, Object viewObject) {
        return viewObject == view;
    }

    @Override
    public Object instantiateItem(final ViewGroup container, int position) {
        LayoutInflater inflater = LayoutInflater.from(mContext);
        ViewGroup layout = (ViewGroup) inflater.inflate(R.layout.brave_stats_tracker_layout,
                           container, false);

        RecyclerView rvTrackers = (RecyclerView) layout.findViewById(R.id.recyclerview);
        // Create adapter passing in the sample user data
        TrackersAdapter adapter = new TrackersAdapter();
        // Attach the adapter to the recyclerview to populate items
        rvTrackers.setAdapter(adapter);
        // Set layout manager to position the items
        rvTrackers.setLayoutManager(new LinearLayoutManager(mContext));

        container.addView(layout);
        return layout;
    }

    @Override
    public void destroyItem(ViewGroup container, int position, Object object) {
        container.removeView((View)object);
    }
}
