/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rate;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.net.Uri;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.graphics.Color;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.RatingBar;
import android.widget.Button;
import android.widget.ImageButton;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.Toast;
import android.support.v4.app.DialogFragment;
import android.app.Dialog;
import android.content.DialogInterface;
import android.util.DisplayMetrics;
import android.content.res.Configuration;

import org.chromium.chrome.R;

import org.chromium.base.task.AsyncTask;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.rate.RateFeedbackUtils;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;

public class RateDialogFragment extends DialogFragment implements View.OnClickListener {
    private static final String SAD = "sad";
    private static final String NEUTRAL = "neutral";

    private String mUserSelection;
	private boolean mIsFeedbackShown;
    private boolean mIsSuccessShown;
    private boolean mIsFromSettings;

	private TextView mRateTitleTextView, mFeedbackTitleTextView;
    private ImageButton mHappyImageButton, mNeutralImageButton, mSadImageButton;
	private EditText mRateFeedbackEditText;
	private Button mPositiveButton, mNegativeButton, mRateButton, mLaterButton;

    private LinearLayout mSmileyLayout, mRateActionLayout;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
        Bundle bundle = getArguments();
        if (bundle != null) {
            mIsFromSettings = bundle.getBoolean(RateUtils.FROM_SETTINGS, false);
        }
	}

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        setDialogParams();
    }

	@Override
	public void onResume() {
	 	super.onResume();
	 	getDialog().setOnKeyListener(new DialogInterface.OnKeyListener() {
	        @Override
	        public boolean onKey(android.content.DialogInterface dialog, 
	                              int keyCode,android.view.KeyEvent event) {
	        	if ((keyCode ==  android.view.KeyEvent.KEYCODE_BACK)) {
	        		dismiss();
	        		return true;
	        	}
	        	else return false;
	        }
	    });
        setDialogParams();
	}
 
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		return inflater.inflate(R.layout.fragment_rate_dialog, container, false);
	}

	@Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        mRateTitleTextView = view.findViewById(R.id.rate_title_tv);
        mFeedbackTitleTextView = view.findViewById(R.id.feedback_title_tv);
        mHappyImageButton = view.findViewById(R.id.happy_ib);
        mNeutralImageButton = view.findViewById(R.id.neutral_ib);
        mSadImageButton = view.findViewById(R.id.sad_ib);
        mRateFeedbackEditText = view.findViewById(R.id.rate_feedback_et);
        mPositiveButton = view.findViewById(R.id.rate_positive_btn);
        mNegativeButton = view.findViewById(R.id.rate_negative_btn);

        mSmileyLayout = view.findViewById(R.id.smiley_layout);
        mRateActionLayout = view.findViewById(R.id.brave_rate_action_layout);

        if(GlobalNightModeStateProviderHolder.getInstance().isInNightMode()) {
            mHappyImageButton.setColorFilter(Color.argb(255, 255, 255, 255));
            mNeutralImageButton.setColorFilter(Color.argb(255, 255, 255, 255));
            mSadImageButton.setColorFilter(Color.argb(255, 255, 255, 255));
        }

        mHappyImageButton.setOnClickListener(this);
        mNeutralImageButton.setOnClickListener(this);
        mSadImageButton.setOnClickListener(this);
        mPositiveButton.setOnClickListener(this);
        mNegativeButton.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        if (view.getId() == R.id.rate_negative_btn) {
            if(mIsFeedbackShown) {
                laterAction();
            } else if(mIsSuccessShown) {
                laterAction();
            }
        } else if (view.getId() == R.id.rate_positive_btn) {
        	if(mIsFeedbackShown) {
        		String feedback = mRateFeedbackEditText.getText().toString().trim();
	            if (TextUtils.isEmpty(feedback)) {
	                Animation shake = AnimationUtils.loadAnimation(getActivity(), R.anim.shake);
	                mRateFeedbackEditText.startAnimation(shake);
	                return;
	            } else {
                    RateFeedbackUtils.RateFeedbackWorkerTask mWorkerTask = new RateFeedbackUtils.RateFeedbackWorkerTask(mUserSelection, feedback, rateFeedbackCallback);
                    mWorkerTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                }
        	} else if (mIsSuccessShown) {
                openPlaystore();
            } else {
                laterAction();
            }
        } else if(view.getId() == R.id.neutral_ib) {
            mUserSelection = NEUTRAL;
            showFeedback();
        } else if(view.getId() == R.id.sad_ib) {
            mUserSelection = SAD;
            showFeedback();
        } else if(view.getId() == R.id.happy_ib) {
            showRateSuccess();
        }
    }

    private void openPlaystore() {
        final Uri marketUri = Uri.parse("market://details?id=" + getActivity().getPackageName());
        try {
            getActivity().startActivity(new Intent(Intent.ACTION_VIEW, marketUri));
        } catch (android.content.ActivityNotFoundException ex) {
            Toast.makeText(getActivity(), "Couldn't find PlayStore on this device", Toast.LENGTH_SHORT).show();
        }
        RateUtils.getInstance(getActivity()).setNextRateDateAndCount();
        dismiss();
    }

    private void laterAction() {
        RateUtils.getInstance(getActivity()).setNextRateDateAndCount();
        dismiss();
    }

    private void showFeedback() {
    	mIsFeedbackShown = true;
        mFeedbackTitleTextView.setVisibility(View.VISIBLE);
        mRateFeedbackEditText.setVisibility(View.VISIBLE);
        mNegativeButton.setVisibility(View.VISIBLE);
        mRateTitleTextView.setVisibility(View.GONE);
        mSmileyLayout.setVisibility(View.GONE);

        mPositiveButton.setText(getResources().getString(R.string.submit));
        mNegativeButton.setText(getResources().getString(android.R.string.cancel));
    }

    private void showRateSuccess() {
        mIsSuccessShown = true;
        mSmileyLayout.setVisibility(View.GONE);
        mNegativeButton.setVisibility(View.VISIBLE);
        mPositiveButton.setText(getResources().getString(R.string.rate));
        mNegativeButton.setText(getResources().getString(R.string.later));

        mRateTitleTextView.setText(getResources().getString(R.string.would_you_mind_leaving_rating));
    }

    private void setDialogParams() {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        int mDeviceHeight = displayMetrics.heightPixels;
        int mDeviceWidth = displayMetrics.widthPixels;

        ViewGroup.LayoutParams params = getDialog().getWindow().getAttributes();
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        boolean isLandscape = ConfigurationUtils.isLandscape(getActivity());
        if(isTablet) {
            params.width = (int) (0.5 * mDeviceWidth);
        } else {
            if(isLandscape) {
                params.width = (int) (0.5 * mDeviceWidth);    
            } else {
                params.width = (int) (0.9 * mDeviceWidth);
            }
        }
        params.height = LinearLayout.LayoutParams.WRAP_CONTENT;
        getDialog().getWindow().setAttributes((android.view.WindowManager.LayoutParams) params);
    }

    private RateFeedbackUtils.RateFeedbackCallback rateFeedbackCallback= new RateFeedbackUtils.RateFeedbackCallback() {
        @Override
        public void rateFeedbackSubmitted() {
            RateUtils.getInstance(getActivity()).setNextRateDateAndCount();
            dismiss();
        }
    };
}