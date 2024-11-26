/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser;

import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;

public class BraveAdFreeCalloutDialogFragment extends BraveDialogFragment {
    private static final String TAG = "BraveAdFreeCallout";

    private ImageView mConfettiImageView;

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (isRemoving() || isDetached()) {
            return;
        }
        checkForImageView();
    }

    @Override
    public void onResume() {
        super.onResume();

        checkForImageView();
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_ad_free_callout_dialog, container, false);

        if (getDialog() != null && getDialog().getWindow() != null) {
            getDialog().getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        }

        return view;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        mConfettiImageView = view.findViewById(R.id.confetti_imageview);

        Button btnVideos = view.findViewById(R.id.btn_videos);
        btnVideos.setOnClickListener(button -> {
            try {
                BraveActivity braveActivity = BraveActivity.getBraveActivity();
                braveActivity.focusSearchBox();
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "onViewCreated btnVideos click " + e);
            }

            dismiss();
        });

        ImageView cancelImageView = view.findViewById(R.id.cancel_imageview);
        cancelImageView.setOnClickListener(button -> { dismiss(); });
    }

    private void checkForImageView() {
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        boolean isLandscape = ConfigurationUtils.isLandscape(getActivity());

        if (mConfettiImageView != null) {
            if (isLandscape && !isTablet) {
                mConfettiImageView.setVisibility(View.GONE);
            } else {
                mConfettiImageView.setVisibility(View.VISIBLE);
            }
        }
    }
}
