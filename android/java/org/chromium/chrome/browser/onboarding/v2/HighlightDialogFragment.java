/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding.v2;

import android.app.Dialog;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.view.WindowManager;

import androidx.fragment.app.DialogFragment;
import androidx.viewpager.widget.ViewPager;

import org.chromium.chrome.R;

import org.chromium.chrome.browser.onboarding.v2.HighlightManager;
import org.chromium.chrome.browser.onboarding.v2.OnboardingV2PagerAdapter;
import org.chromium.chrome.browser.onboarding.v2.HighlightDialogListener;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;

public class HighlightDialogFragment extends DialogFragment{
    private HighlightManager highlightManager;
    private HighlightItem item;
    private HighlightView highlightView;
    private ViewPager viewpager;

    public void setHighlightItem(HighlightItem item) {
        this.item = item;
        if (highlightView != null) {
            highlightView.setHighlightItem(item);
            highlightView.invalidate();
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Dialog dialog = new Dialog(getActivity(), android.R.style.Theme_Black_NoTitleBar);
        dialog.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        dialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND);
        return dialog;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.highlight_layout, container);
        highlightView = view.findViewById(R.id.highlight_view);
        highlightView.setHighlightItem(item);

        // tab slider
        OnboardingV2PagerAdapter onboardingV2PagerAdapter = new OnboardingV2PagerAdapter(getChildFragmentManager());
        onboardingV2PagerAdapter.setHighlightListener(highlightDialogListener);

        // Set up the ViewPager with the sections adapter.
        viewpager = view.findViewById(R.id.viewpager);
        viewpager.setAdapter(onboardingV2PagerAdapter);
        viewpager.addOnPageChangeListener(new ViewPager.OnPageChangeListener() {
            @Override
            public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
                if (positionOffset == 0) {
                    Log.e("HighlightDialogFragment", "onPageScrolled : " + position);
                    if (position == 0) {
                        highlightManager.showHighlight(R.id.brave_stats_ads, 0);
                    } else if (position == 1) {
                        highlightManager.showHighlight(R.id.brave_stats_data_saved, 1);
                    } else if (position == 2) {
                        highlightManager.showHighlight(R.id.brave_stats_time, 2);
                    }
                }
            }

            @Override
            public void onPageSelected(int position) {
                Log.e("HighlightDialogFragment", "onPageSelected : " + position);
            }

            @Override
            public void onPageScrollStateChanged(int state) {

            }
        });

        ImageView btnClose = view.findViewById(R.id.onboarding_bottom_sheet_close);
        btnClose.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dismiss();
            }
        });

        OnboardingPrefManager.getInstance().setNewOnboardingShown(true);

        return view;
    }

    @Override
    public void onDestroyView() {
        /*
         * https://code.google.com/p/android/issues/detail?id=17423
         */
        if (getDialog() != null && getRetainInstance()) {
            getDialog().setDismissMessage(null);
        }
        super.onDestroyView();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // setCancelable(false);
        // setHasOptionsMenu(false);
    }

    public void setManager(HighlightManager highlightManager) {
        this.highlightManager = highlightManager;
    }

    private HighlightDialogListener highlightDialogListener = new HighlightDialogListener() {
        @Override
        public void onNextPage() {
            if (viewpager != null) {
                int currentPage = viewpager.getCurrentItem();
                if (currentPage == viewpager.getAdapter().getCount() - 1
                        && OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
                    dismiss();
                    BraveStatsUtil.showBraveStats();
                } else{
                    viewpager.setCurrentItem(currentPage + 1);
                }
            }
        }
    };
}