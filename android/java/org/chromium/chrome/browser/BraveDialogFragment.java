/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import android.app.Dialog;
import android.content.res.Configuration;
import android.util.DisplayMetrics;
import android.view.Window;
import android.view.WindowManager;
import android.widget.LinearLayout;

import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.util.ConfigurationUtils;
import org.chromium.ui.base.DeviceFormFactor;

/** Brave's extension for DialogFragment */
@NullMarked
public class BraveDialogFragment extends DialogFragment {
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (isRemoving() || isDetached()) {
            return;
        }
        setDialogParams();
    }

    @Override
    public void onResume() {
        super.onResume();

        Dialog dialog = getDialog();
        if (dialog != null) {
            dialog.setCanceledOnTouchOutside(false);
        }

        setDialogParams();
    }

    @Override
    public void show(FragmentManager manager, @Nullable String tag) {
        try {
            super.show(manager, tag);
        } catch (IllegalStateException ignored) {
            FragmentTransaction ft = manager.beginTransaction();
            ft.add(this, tag);
            ft.commitAllowingStateLoss();
        }
    }

    private void setDialogParams() {
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        int mDeviceWidth = displayMetrics.widthPixels;

        Dialog dialog = getDialog();
        if (dialog == null) return;

        Window window = dialog.getWindow();
        if (window == null) return;

        WindowManager.LayoutParams params = window.getAttributes();
        boolean isTablet = DeviceFormFactor.isNonMultiDisplayContextOnTablet(getActivity());
        boolean isLandscape = ConfigurationUtils.isLandscape(getActivity());
        if (isTablet || isLandscape) {
            params.width = (int) (0.5 * mDeviceWidth);
        } else {
            params.width = (int) (0.9 * mDeviceWidth);
        }
        params.height = LinearLayout.LayoutParams.WRAP_CONTENT;
        window.setAttributes(params);
    }
}
