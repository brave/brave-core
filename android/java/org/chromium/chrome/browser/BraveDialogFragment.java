/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser;

import android.content.res.Configuration;
import android.util.DisplayMetrics;
import android.view.WindowManager;
import android.widget.LinearLayout;

import androidx.fragment.app.DialogFragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;

public class BraveDialogFragment extends DialogFragment {
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        setDialogParams();
    }

    @Override
    public void onResume() {
        super.onResume();

        getDialog().setCanceledOnTouchOutside(false);

        setDialogParams();
    }

    private void setDialogParams() {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        int mDeviceWidth = displayMetrics.widthPixels;

        WindowManager.LayoutParams params = getDialog().getWindow().getAttributes();
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        boolean isLandscape = ConfigurationUtils.isLandscape(getActivity());
        if (isTablet || isLandscape) {
            params.width = (int) (0.5 * mDeviceWidth);
        } else {
            params.width = (int) (0.9 * mDeviceWidth);
        }
        params.height = LinearLayout.LayoutParams.WRAP_CONTENT;
        getDialog().getWindow().setAttributes(params);
    }
}
