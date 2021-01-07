/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.vpn;

import android.content.DialogInterface;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;

import androidx.fragment.app.DialogFragment;

import org.chromium.chrome.R;

import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.chrome.browser.util.ConfigurationUtils;

public class VpnCalloutDialogFragment extends DialogFragment implements View.OnClickListener {

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
        return inflater.inflate(R.layout.fragment_vpn_callout_dialog, container, false);
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        Button mEnableButton = view.findViewById(R.id.btn_enable);
        mEnableButton.setOnClickListener(this);

        ImageView btnClose = view.findViewById(R.id.modal_close);
        btnClose.setOnClickListener(this);
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
