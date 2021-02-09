/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding.v2;

import android.content.Context;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import android.os.Handler;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.widget.Button;
import android.widget.ImageView;
import android.view.Gravity;
import android.animation.ValueAnimator;
import com.airbnb.lottie.LottieAnimationView;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.onboarding.v2.HighlightDialogFragment.HighlightDialogListener;

import org.chromium.chrome.R;

import java.util.List;
import java.util.Arrays;

/**
 * A simple {@link Fragment} subclass.
 */
public class OnboardingV2Fragment extends Fragment {

	private Context mContext;
	private static final List<String> mHeaders = Arrays.asList(
	            ContextUtils.getApplicationContext().getResources().getString(R.string.privacy_protection),
	            ContextUtils.getApplicationContext().getResources().getString(R.string.save_data_and_battery),
	            ContextUtils.getApplicationContext().getResources().getString(R.string.websites_load_faster),
	            ContextUtils.getApplicationContext().getResources().getString(R.string.get_weekly_updates)
	        );
	private static final List<String> mTexts = Arrays.asList(
	            ContextUtils.getApplicationContext().getResources().getString(R.string.privacy_protection_text),
	            ContextUtils.getApplicationContext().getResources().getString(R.string.save_data_and_battery_text),
	            ContextUtils.getApplicationContext().getResources().getString(R.string.websites_load_faster_text),
	            ContextUtils.getApplicationContext().getResources().getString(R.string.get_weekly_updates_text)
	        );

	private static final List<String> mAnimations = Arrays.asList(
	            "privacy_protection.json",
	            "save_data_and_battery.json",
	            "website_loads_faster.json",
	            null
	        );

	private int mPosition;
	private HighlightDialogListener highlightDialogListener;
	private boolean isFromStats;

	private LottieAnimationView mAnimatedView;
	private Button mAction;
	private Button mLearnMoreButton;
	private View mIndicator1;
	private View mIndicator2;
	private View mIndicator3;
	private View mIndicator4;

	public OnboardingV2Fragment() {
		// Required empty public constructor
	}

	@Override
	public void setUserVisibleHint(boolean isVisibleToUser) {
		super.setUserVisibleHint(isVisibleToUser);
		if (isVisibleToUser) {
			new Handler().postDelayed(new Runnable() {
				@Override
				public void run() {
					if (mAnimatedView != null) {
						mAnimatedView.playAnimation();
					}
				}
			}, 200);
			updateActionText();
		}
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		mContext = ContextUtils.getApplicationContext();
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
	                         Bundle savedInstanceState) {
		// Inflate the layout for this fragment
		return inflater.inflate(R.layout.fragment_onboarding_v2, container, false);
	}

	@Override
	public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
		super.onViewCreated(view, savedInstanceState);

		LinearLayout mOnboardingLayout = view.findViewById(R.id.onboarding_layout);
		view.findViewById(R.id.indicator_layout).setVisibility(View.VISIBLE);

		TextView mHeader = view.findViewById(R.id.onboarding_header);
		mHeader.setText(mHeaders.get(mPosition));

		TextView mText = view.findViewById(R.id.onboarding_text);
		mText.setText(mTexts.get(mPosition));

		mAction = view.findViewById(R.id.btn_turn_on_privacy_stats);
		mAction.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (highlightDialogListener != null)
					highlightDialogListener.onNextPage();
			}
		});

		mLearnMoreButton = view.findViewById(R.id.btn_learn_more);
		mLearnMoreButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (highlightDialogListener != null)
					highlightDialogListener.onLearnMore();
			}
		});

		updateActionText();

		mAnimatedView = view.findViewById(R.id.onboarding_image);
		if (mAnimations.get(mPosition) != null) {
			mAnimatedView.setVisibility(View.VISIBLE);
			mAnimatedView.setAnimation(mAnimations.get(mPosition));
			mAnimatedView.setRepeatCount(ValueAnimator.INFINITE);
		}

		mIndicator1 = view.findViewById(R.id.indicator_1);
		mIndicator2 = view.findViewById(R.id.indicator_2);
		mIndicator3 = view.findViewById(R.id.indicator_3);
		mIndicator4 = view.findViewById(R.id.indicator_4);

		switch (mPosition) {
		case 0:
			mIndicator1.setBackground(mContext.getResources().getDrawable(R.drawable.selected_indicator));
			break;
		case 1:
			mIndicator2.setBackground(mContext.getResources().getDrawable(R.drawable.selected_indicator));
			break;
		case 2:
			mIndicator3.setBackground(mContext.getResources().getDrawable(R.drawable.selected_indicator));
			break;
		case 3:
			mIndicator4.setBackground(mContext.getResources().getDrawable(R.drawable.selected_indicator));
			FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(
			    FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
			params.gravity = Gravity.CENTER;
			mOnboardingLayout.setLayoutParams(params);
			if (isFromStats)
				mLearnMoreButton.setVisibility(View.VISIBLE);
			break;
		}
	}

	public void setPosition(int position) {
		this.mPosition = position;
	}

	public void setHighlightListener(HighlightDialogListener highlightDialogListener) {
		this.highlightDialogListener = highlightDialogListener;
	}

	public void setFromStats(boolean isFromStats) {
        this.isFromStats = isFromStats;
    }

	private void updateActionText() {
		if (mAction != null) {
			if (OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
				mAction.setText(mContext.getResources().getString(R.string.next));
				if (mIndicator4 != null)
					mIndicator4.setVisibility(View.GONE);
			} else {
				mAction.setText(mContext.getResources().getString(R.string.turn_on_privacy_stats));
				if (mIndicator4 != null)
					mIndicator4.setVisibility(View.VISIBLE);
			}
		}
	}
}