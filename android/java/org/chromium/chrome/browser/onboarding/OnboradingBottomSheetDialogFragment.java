/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.onboarding;

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

public class OnboradingBottomSheetDialogFragment extends BottomSheetDialogFragment {
    private int mOnboardingType;
    private NewTabPageListener mNewTabPageListener;

    public static OnboradingBottomSheetDialogFragment newInstance() {
        return new OnboradingBottomSheetDialogFragment();
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater,
                             @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        mOnboardingType = getArguments() != null ?
                          getArguments().getInt(OnboardingPrefManager.ONBOARDING_TYPE , OnboardingPrefManager.ONBOARDING_INVALID_OPTION) :
                          OnboardingPrefManager.ONBOARDING_INVALID_OPTION;

        return inflater.inflate(R.layout.onboarding_bottom_sheet, container, false);
    }

    @Override
    public void onPause() {
        // newTabPageListener.updateInteractableFlag(true);
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

    public void setNewTabPageListener(NewTabPageListener newTabPageListener) {
        this.mNewTabPageListener = newTabPageListener;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        view.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                BottomSheetDialog dialog = (BottomSheetDialog) getDialog();
                FrameLayout bottomSheet = (FrameLayout) dialog.findViewById(com.google.android.material.R.id.design_bottom_sheet);
                BottomSheetBehavior behavior = BottomSheetBehavior.from(bottomSheet);
                behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
            }
        });

        ViewPager pager = (ViewPager) view.findViewById(R.id.viewpager);
        OnboardingBottomSheetViewPagerAdapter adapter = new OnboardingBottomSheetViewPagerAdapter(mOnboardingType, mNewTabPageListener);
        pager.setAdapter(adapter);

        TabLayout tabLayout = (TabLayout) view.findViewById(R.id.tab_layout);
        tabLayout.setupWithViewPager(pager, true);

        ImageView btnClose = view.findViewById(R.id.onboarding_bottom_sheet_close);
        btnClose.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dismiss();
            }
        });
    }
}
