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

import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.chrome.browser.util.ConfigurationUtils;

public class RateDialogFragment extends DialogFragment implements View.OnClickListener {
	private boolean mIsFeedbackShown;
    private boolean mIsFromSettings;

	private TextView mRateTitleTextView, mFeedbackTitleTextView;
    private ImageButton mHappyImageButton, mNeutralImageButton, mSadImageButton;
	private EditText mRateFeedbackEditText;
	private Button mPositiveButton, mNegativeButton, mRateButton, mLaterButton;

    private LinearLayout mSmileyLayout, mRateSuccessActionLayout, mRateActionLayout;

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
        mRateButton = view.findViewById(R.id.rate_btn);
        mLaterButton = view.findViewById(R.id.later_btn);

        mSmileyLayout = view.findViewById(R.id.smiley_layout);
        mRateSuccessActionLayout = view.findViewById(R.id.brave_rate_success_action_layout);
        mRateActionLayout = view.findViewById(R.id.brave_rate_action_layout);

        mHappyImageButton.setOnClickListener(this);
        mNeutralImageButton.setOnClickListener(this);
        mSadImageButton.setOnClickListener(this);
        mPositiveButton.setOnClickListener(this);
        mNegativeButton.setOnClickListener(this);
        mRateButton.setOnClickListener(this);
        mLaterButton.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        if (view.getId() == R.id.rate_negative_btn) {
            if(mIsFeedbackShown) {
                laterAction();
            } else {
                showNever();
            }
            dismiss();
        } else if (view.getId() == R.id.rate_positive_btn) {
        	if(mIsFeedbackShown) {
        		String feedback = mRateFeedbackEditText.getText().toString().trim();
	            if (TextUtils.isEmpty(feedback)) {
	                Animation shake = AnimationUtils.loadAnimation(getActivity(), R.anim.shake);
	                mRateFeedbackEditText.startAnimation(shake);
	                return;
	            }
	            dismiss();
	            showNever();
        	} else {
                laterAction();
                dismiss();
            }
        } else if(view.getId() == R.id.neutral_ib || view.getId() == R.id.sad_ib) {
            showFeedback();
        } else if(view.getId() == R.id.happy_ib) {
            showRateSuccess();
        } else if(view.getId() == R.id.rate_btn) {
            openPlaystore();
            dismiss();
        } else if(view.getId() == R.id.later_btn) {
            laterAction();
            dismiss();
        }
    }

    private void openPlaystore() {
        final Uri marketUri = Uri.parse("market://details?id=" + getActivity().getPackageName());
        try {
            getActivity().startActivity(new Intent(Intent.ACTION_VIEW, marketUri));
        } catch (android.content.ActivityNotFoundException ex) {
            Toast.makeText(getActivity(), "Couldn't find PlayStore on this device", Toast.LENGTH_SHORT).show();
        }
    }

    private void showNever() {
        RateUtils.getInstance(getActivity()).setPrefRateEnabled(false);
    }

    private void laterAction() {
        RateUtils.getInstance(getActivity()).setNextRateDateAndCount();
    }

    private void showFeedback() {
    	mIsFeedbackShown = true;
        mFeedbackTitleTextView.setVisibility(View.VISIBLE);
        mRateFeedbackEditText.setVisibility(View.VISIBLE);
        mRateTitleTextView.setVisibility(View.GONE);
        mSmileyLayout.setVisibility(View.GONE);

        mPositiveButton.setText(getResources().getString(R.string.submit));
        mNegativeButton.setText(getResources().getString(android.R.string.cancel));
    }

    private void showRateSuccess() {
        mSmileyLayout.setVisibility(View.GONE);
        mRateActionLayout.setVisibility(View.GONE);
        mRateSuccessActionLayout.setVisibility(View.VISIBLE);

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
}