/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved. This Source Code Form is subject to
 * the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with
 * this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser.omnibox;

import android.app.Activity;
import android.app.Dialog;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.util.Patterns;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import androidx.fragment.app.DialogFragment;

import com.google.android.gms.vision.barcode.Barcode;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.qrreader.CameraSourcePreview;
import org.chromium.chrome.browser.qrreader.QRCodeCameraManager;
import org.chromium.chrome.browser.util.BraveTouchUtils;

public class BraveLocationBarQRDialogFragment extends DialogFragment
        implements QRCodeCameraManager.Callback, QRCodeCameraManager.HostProvider {
    private static final String TAG = "Scan QR Code Dialog";

    private QRCodeCameraManager mCameraManager;
    private LocationBarMediator mLocationBarMediator;

    // The Android Fragment framework requires a zero-argument constructor to
    // instantiate fragments. It usually happens on a fragment re-creation
    // https://github.com/brave/brave-browser/issues/41454
    public BraveLocationBarQRDialogFragment() {
        super();
    }

    private BraveLocationBarQRDialogFragment(LocationBarMediator locationBarMediator) {
        mLocationBarMediator = locationBarMediator;
    }

    public static BraveLocationBarQRDialogFragment newInstance(
            LocationBarMediator locationBarMediator) {
        return new BraveLocationBarQRDialogFragment(locationBarMediator);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // That could happen on a fragment re-creation, not a big deal if we don't
        // show the QR code dialog as everything gets re-created at that stage
        if (mLocationBarMediator == null) {
            dismiss();

            return null;
        }
        View view =
                inflater.inflate(R.layout.fragment_brave_location_bar_qr_dialog, container, false);

        if (getDialog() != null && getDialog().getWindow() != null) {
            getDialog().getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        }

        return view;
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        ImageView backImageView = (ImageView) view.findViewById(R.id.back_imageview);
        backImageView.setOnClickListener(
                imageview -> {
                    dismiss();
                });

        CameraSourcePreview cameraSourcePreview =
                (CameraSourcePreview) view.findViewById(R.id.preview);

        mCameraManager = new QRCodeCameraManager(this, this);
        mCameraManager.init(cameraSourcePreview);
        mCameraManager.createCameraSource();

        BraveTouchUtils.ensureMinTouchTarget(backImageView);

        try {
            mCameraManager.startCameraSource();
        } catch (SecurityException exc) {
            Log.e(TAG, "Security exception when starting camera source", exc);
        }
    }

    @Override
    public void onDestroy() {
        if (mCameraManager != null) {
            mCameraManager.release();
        }
        super.onDestroy();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (newConfig.orientation != Configuration.ORIENTATION_UNDEFINED
                && mCameraManager != null) {
            mCameraManager.handleConfigurationChange();
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        try {
            if (mCameraManager != null) {
                mCameraManager.startCameraSource();
            }
        } catch (SecurityException se) {
            Log.e(TAG, "Do not have permission to start the camera", se);
        } catch (RuntimeException e) {
            Log.e(TAG, "Could not start camera source.", e);
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mCameraManager != null) {
            mCameraManager.stopCameraSource();
        }
    }

    // QRCodeCameraManager.Callback implementation
    @Override
    public void onDetectedQrCode(Barcode barcode) {
        if (barcode == null) {
            return;
        }

        if (getActivity() != null) {
            final String barcodeValue = barcode.displayValue;
            getActivity()
                    .runOnUiThread(
                            () -> {
                                if (Patterns.WEB_URL.matcher(barcodeValue).matches()) {
                                    mLocationBarMediator.setSearchQuery(barcodeValue);
                                } else {
                                    mLocationBarMediator.performSearchQuery(barcodeValue, null);
                                }
                            });

            dismiss();
        }
    }

    @Override
    public boolean onPlayServicesUnavailable(Dialog errorDialog) {
        if (errorDialog != null) {
            errorDialog.show();
        }
        dismiss();
        return true;
    }

    // QRCodeCameraManager.HostProvider implementation
    @Override
    public Activity getHostActivity() {
        return getActivity();
    }

    @Override
    public boolean isHostValid() {
        return getActivity() != null && !isRemoving() && !isDetached();
    }
}
