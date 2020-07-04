/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_stats;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.google.android.material.tabs.TabLayout;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;
import android.widget.ImageView;
import android.widget.FrameLayout;
import android.content.res.Configuration;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import android.view.ViewTreeObserver;

import org.chromium.chrome.R;

import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.chrome.browser.ntp_background_images.util.NewTabPageListener;
import org.chromium.chrome.browser.onboarding.OnboardingBottomSheetViewPagerAdapter;

import static org.chromium.ui.base.ViewUtils.dpToPx;

public class BraveStatsBottomSheetDialogFragment extends BottomSheetDialogFragment {
    private ViewPager mViewPager;

    public static BraveStatsBottomSheetDialogFragment newInstance() {
        return new BraveStatsBottomSheetDialogFragment();
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater,
                             @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.brave_stats_bottom_sheet, container, false);
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        if (isTablet || (!isTablet && newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE)) {
            getDialog().getWindow().setLayout(dpToPx(getActivity(), 400), -1);
        } else {
            getDialog().getWindow().setLayout(-1, -1);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        if (isTablet || (!isTablet && ConfigurationUtils.isLandscape(getActivity()))) {
            getDialog().getWindow().setLayout(dpToPx(getActivity(), 400), -1);
        } else {
            getDialog().getWindow().setLayout(-1, -1);
        }
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        // view.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
        //     @Override
        //     public void onGlobalLayout() {
        //         BottomSheetDialog dialog = (BottomSheetDialog) getDialog();
        //         FrameLayout bottomSheet = (FrameLayout) dialog.findViewById(com.google.android.material.R.id.design_bottom_sheet);
        //         BottomSheetBehavior behavior = BottomSheetBehavior.from(bottomSheet);
        //         behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
        //     }
        // });

        mViewPager = (ViewPager) view.findViewById(R.id.viewpager);
        BraveStatsBottomSheetViewPagerAdapter adapter = new BraveStatsBottomSheetViewPagerAdapter();
        mViewPager.setAdapter(adapter);
        mViewPager.setCurrentItem(0);

        mViewPager.addOnPageChangeListener(new ViewPager.OnPageChangeListener() {
            @Override
            public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {

            }

            @Override
            public void onPageSelected(int position) {

            }

            @Override
            public void onPageScrollStateChanged(int state) {

            }
        });

        TabLayout tabLayout = (TabLayout) view.findViewById(R.id.tablayout);
        tabLayout.setupWithViewPager(mViewPager);

        ImageView btnClose = view.findViewById(R.id.brave_stats_bottom_sheet_close);
        btnClose.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dismiss();
            }
        });
    }
}
