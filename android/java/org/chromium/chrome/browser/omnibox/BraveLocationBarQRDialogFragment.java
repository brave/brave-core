/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.omnibox;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.hardware.Camera;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.URLUtil;
import android.widget.ImageView;

import androidx.fragment.app.DialogFragment;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.vision.MultiProcessor;
import com.google.android.gms.vision.barcode.Barcode;
import com.google.android.gms.vision.barcode.BarcodeDetector;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.qrreader.BarcodeTracker;
import org.chromium.chrome.browser.qrreader.BarcodeTrackerFactory;
import org.chromium.chrome.browser.qrreader.CameraSource;
import org.chromium.chrome.browser.qrreader.CameraSourcePreview;
import org.chromium.chrome.browser.util.BraveTouchUtils;

import java.io.IOException;

public class BraveLocationBarQRDialogFragment
        extends DialogFragment implements BarcodeTracker.BarcodeGraphicTrackerCallback {
    private static final String TAG = "Scan QR Code Dialog";

    private CameraSource mCameraSource;
    private CameraSourcePreview mCameraSourcePreview;
    private LocationBarMediator mLocationBarMediator;

    // Intent request code to handle updating play services if needed.
    private static final int RC_HANDLE_GMS = 9001;

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
        backImageView.setOnClickListener(imageview -> { dismiss(); });

        mCameraSourcePreview = (CameraSourcePreview) view.findViewById(R.id.preview);
        createCameraSource(true, false);

        BraveTouchUtils.ensureMinTouchTarget(backImageView);

        try {
            startCameraSource();
        } catch (SecurityException exc) {
        }
    }

    @SuppressLint("InlinedApi")
    private void createCameraSource(boolean autoFocus, boolean useFlash) {
        Context context = getActivity().getApplicationContext();
        // A barcode detector is created to track barcodes.  An associated multi-processor instance
        // is set to receive the barcode detection results, track the barcodes, and maintain
        // graphics for each barcode on screen.  The factory is used by the multi-processor to
        // create a separate tracker instance for each barcode.
        BarcodeDetector barcodeDetector =
                new BarcodeDetector.Builder(context).setBarcodeFormats(Barcode.ALL_FORMATS).build();
        BarcodeTrackerFactory barcodeFactory = new BarcodeTrackerFactory(this);
        barcodeDetector.setProcessor(new MultiProcessor.Builder<>(barcodeFactory).build());

        if (!barcodeDetector.isOperational()) {
            // Note: The first time that an app using the barcode or face API is installed on a
            // device, GMS will download a native libraries to the device in order to do detection.
            // Usually this completes before the app is run for the first time.  But if that
            // download has not yet completed, then the above call will not detect any barcodes.
            //
            // isOperational() can be used to check if the required native libraries are currently
            // available.  The detectors will automatically become operational once the library
            // downloads complete on device.
            Log.w(TAG, "Detector dependencies are not yet available.");
        }

        // Creates and starts the camera.  Note that this uses a higher resolution in comparison
        // to other detection examples to enable the barcode detector to detect small barcodes
        // at long distances.
        DisplayMetrics metrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);

        CameraSource.Builder builder =
                new CameraSource.Builder(context, barcodeDetector)
                        .setFacing(CameraSource.CAMERA_FACING_BACK)
                        .setRequestedPreviewSize(metrics.widthPixels, metrics.heightPixels)
                        .setRequestedFps(24.0f);

        // Make sure that auto focus is an available option
        builder = builder.setFocusMode(
                autoFocus ? Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE : null);

        mCameraSource =
                builder.setFlashMode(useFlash ? Camera.Parameters.FLASH_MODE_TORCH : null).build();
    }

    private void startCameraSource() throws SecurityException {
        if (mCameraSource != null && mCameraSourcePreview.mCameraExist) {
            // Check that the device has play services available.
            try {
                int code = GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(
                        getActivity().getApplicationContext());
                if (code != ConnectionResult.SUCCESS) {
                    Dialog dlg = GoogleApiAvailability.getInstance().getErrorDialog(
                            getActivity(), code, RC_HANDLE_GMS);
                    if (null != dlg) {
                        dlg.show();
                    }

                    dismiss();
                }
            } catch (ActivityNotFoundException e) {
                Log.e(TAG, "Unable to start camera source.", e);
                mCameraSource.release();
                mCameraSource = null;

                return;
            }
            try {
                mCameraSourcePreview.start(mCameraSource);
            } catch (IOException e) {
                Log.e(TAG, "Unable to start camera source.", e);
                mCameraSource.release();
                mCameraSource = null;
            }
        }
    }

    @Override
    public void onDestroy() {
        if (mCameraSourcePreview != null) {
            mCameraSourcePreview.release();
        }
        super.onDestroy();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        // Checks the orientation of the screen
        if (newConfig.orientation != Configuration.ORIENTATION_UNDEFINED
                && null != mCameraSourcePreview) {
            mCameraSourcePreview.stop();
            try {
                startCameraSource();
            } catch (SecurityException exc) {
            }
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        try {
            if (mCameraSourcePreview != null) {
                startCameraSource();
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
        if (mCameraSourcePreview != null) {
            mCameraSourcePreview.stop();
        }
    }

    @Override
    public void onDetectedQrCode(Barcode barcode) {
        if (barcode == null) {
            return;
        }

        if (getActivity() != null) {
            final String barcodeValue = barcode.displayValue;
            getActivity().runOnUiThread(() -> {
                if (URLUtil.isNetworkUrl(barcodeValue)) {
                    mLocationBarMediator.setSearchQuery(barcodeValue);
                } else {
                    mLocationBarMediator.performSearchQuery(barcodeValue, null);
                }
            });

            dismiss();
        }
    }
}
