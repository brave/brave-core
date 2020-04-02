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
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.Toast;
import android.support.v4.app.DialogFragment;
import android.content.DialogInterface;
import android.util.DisplayMetrics;
import android.content.res.Configuration;

import org.chromium.chrome.R;

import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.chrome.browser.util.ConfigurationUtils;

public class RateDialogFragment extends DialogFragment implements RatingBar.OnRatingBarChangeListener, View.OnClickListener {

	private static final float RATE_THRESHOLD = 5.0f;

	private boolean mIsFeedbackShown;

	private TextView mRateTitleTextView, mFeedbackTitleTextView;
	private RatingBar mRateBar;
	private EditText mRateFeedbackEditText;
	private Button mPositiveButton, mNegativeButton;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
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
        mRateBar = view.findViewById(R.id.brave_rb);
        mRateFeedbackEditText = view.findViewById(R.id.rate_feedback_et);
        mPositiveButton = view.findViewById(R.id.rate_positive_btn);
        mNegativeButton = view.findViewById(R.id.rate_negative_btn);

        mRateBar.setOnRatingBarChangeListener(this);
        mPositiveButton.setOnClickListener(this);
        mNegativeButton.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        if (view.getId() == R.id.rate_negative_btn) {
            dismiss();
            showNever();
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
        	}
        	dismiss();
        }
    }

    @Override
    public void onRatingChanged(RatingBar ratingBar, float v, boolean b) {
        if (ratingBar.getRating() == RATE_THRESHOLD) {
            openPlaystore();
            dismiss();
        } else {
            showFeedback();
        }
        showNever();
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
        // sharedpreferences = context.getSharedPreferences(MyPrefs, Context.MODE_PRIVATE);
        // SharedPreferences.Editor editor = sharedpreferences.edit();
        // editor.putBoolean(SHOW_NEVER, true);
        // editor.commit();
    }

    private void showFeedback() {
    	mIsFeedbackShown = true;
        mFeedbackTitleTextView.setVisibility(View.VISIBLE);
        mRateFeedbackEditText.setVisibility(View.VISIBLE);
        mRateTitleTextView.setVisibility(View.GONE);
        mRateBar.setVisibility(View.GONE);

        mPositiveButton.setText(getResources().getString(R.string.submit));
        mNegativeButton.setText(getResources().getString(android.R.string.cancel));
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