/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser;

import android.content.DialogInterface;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.widget.LinearLayout;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;

import androidx.fragment.app.DialogFragment;

import org.chromium.chrome.R;

import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.chrome.browser.BraveRewardsHelper;

public class CrossPromotionalModalDialogFragment extends DialogFragment implements View.OnClickListener {

    private static final String BRAVE_URL = "brave.com";

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
                                 int keyCode, android.view.KeyEvent event) {
                if ((keyCode == android.view.KeyEvent.KEYCODE_BACK)) {
                    dismiss();
                    return true;
                } else return false;
            }
        });
        setDialogParams();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_cross_promotional_modal_dialog, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        Button mDoneButton = view.findViewById(R.id.btn_done);
        mDoneButton.setOnClickListener(this);

        String mPromoText = getResources().getString(R.string.download_brave);
        int indexOfSpan = mPromoText.indexOf(BRAVE_URL);

        TextView mCrossPromoText = view.findViewById(R.id.cross_modal_text);

        Spanned textToSpan = BraveRewardsHelper.spannedFromHtmlString(mPromoText);
        SpannableString ss = new SpannableString(textToSpan.toString());

        ForegroundColorSpan foregroundSpan =
            new ForegroundColorSpan(getResources().getColor(R.color.shield_back_button_tint));
        ss.setSpan(foregroundSpan, indexOfSpan,
                   (indexOfSpan + BRAVE_URL.length()), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        mCrossPromoText.setText(ss);
    }

    @Override
    public void onClick(View view) {
        dismiss();
    }

    private void setDialogParams() {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        int mDeviceHeight = displayMetrics.heightPixels;
        int mDeviceWidth = displayMetrics.widthPixels;

        WindowManager.LayoutParams params = getDialog().getWindow().getAttributes();
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        boolean isLandscape = ConfigurationUtils.isLandscape(getActivity());
        if (isTablet) {
            params.width = (int) (0.5 * mDeviceWidth);
        } else {
            if (isLandscape) {
                params.width = (int) (0.5 * mDeviceWidth);
            } else {
                params.width = (int) (0.9 * mDeviceWidth);
            }
        }
        params.height = LinearLayout.LayoutParams.WRAP_CONTENT;
        getDialog().getWindow().setAttributes(params);
    }
}
