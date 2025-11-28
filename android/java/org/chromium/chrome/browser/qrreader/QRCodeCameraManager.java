/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.qrreader;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.hardware.Camera;
import android.hardware.display.DisplayManager;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.Surface;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.vision.MultiProcessor;
import com.google.android.gms.vision.barcode.Barcode;
import com.google.android.gms.vision.barcode.BarcodeDetector;

import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

import java.io.IOException;

/**
 * Helper class to manage QR code camera operations. This class encapsulates camera source creation,
 * lifecycle management, and 180-degree rotation handling for QR code scanning.
 */
@NullMarked
public class QRCodeCameraManager implements BarcodeTracker.BarcodeGraphicTrackerCallback {
    private static final String TAG = "QRCodeCameraManager";
    private static final int INITIAL_ROTATION = -1;
    private static final int RC_HANDLE_GMS = 9001;

    /** Callback interface for QR code detection and error handling. */
    public interface Callback {
        /** Called when a QR code is detected. */
        void onDetectedQrCode(Barcode barcode);

        /** Called when Google Play Services is not available. Returns true if handled. */
        default boolean onPlayServicesUnavailable(@Nullable Dialog errorDialog) {
            return false;
        }
    }

    /** Interface for providing activity context and fragment state. */
    public interface HostProvider {
        Activity getHostActivity();

        boolean isHostValid();
    }

    private @Nullable CameraSource mCameraSource;
    private @Nullable CameraSourcePreview mCameraSourcePreview;
    private @Nullable DisplayManager mDisplayManager;
    private int mLastRotation = INITIAL_ROTATION;
    private final Callback mCallback;
    private final HostProvider mHostProvider;

    private final DisplayManager.DisplayListener mDisplayListener =
            new DisplayManager.DisplayListener() {
                @Override
                public void onDisplayAdded(int displayId) {}

                @Override
                public void onDisplayRemoved(int displayId) {}

                @Override
                public void onDisplayChanged(int displayId) {
                    if (!mHostProvider.isHostValid()) {
                        return;
                    }
                    Activity activity = mHostProvider.getHostActivity();
                    if (activity == null) {
                        return;
                    }
                    Display display = activity.getWindowManager().getDefaultDisplay();
                    if (display == null || display.getDisplayId() != displayId) {
                        return;
                    }
                    int currentRotation = display.getRotation();
                    if (mLastRotation != INITIAL_ROTATION
                            && is180DegreeRotation(mLastRotation, currentRotation)) {
                        recreateCameraSource();
                    }
                    mLastRotation = currentRotation;
                }
            };

    /**
     * Creates a new QRCodeCameraManager.
     *
     * @param callback The callback for QR code detection events.
     * @param hostProvider The provider for activity context and fragment state.
     */
    public QRCodeCameraManager(Callback callback, HostProvider hostProvider) {
        mCallback = callback;
        mHostProvider = hostProvider;
    }

    /**
     * Initializes the camera manager with a preview view.
     *
     * @param cameraSourcePreview The preview view for camera output.
     */
    public void init(CameraSourcePreview cameraSourcePreview) {
        mCameraSourcePreview = cameraSourcePreview;

        Activity activity = mHostProvider.getHostActivity();
        if (activity != null) {
            mDisplayManager = (DisplayManager) activity.getSystemService(Context.DISPLAY_SERVICE);
            if (mDisplayManager != null) {
                Display display = activity.getWindowManager().getDefaultDisplay();
                if (display != null) {
                    mLastRotation = display.getRotation();
                }
                mDisplayManager.registerDisplayListener(mDisplayListener, null);
            }
        }
    }

    /** Creates the camera source with auto-focus enabled and flash disabled. */
    public void createCameraSource() {
        createCameraSource(true, false);
    }

    /**
     * Creates the camera source for barcode detection.
     *
     * @param autoFocus Whether to enable auto-focus.
     * @param useFlash Whether to enable flash.
     */
    @SuppressLint("InlinedApi")
    public void createCameraSource(boolean autoFocus, boolean useFlash) {
        Activity activity = mHostProvider.getHostActivity();
        if (activity == null) {
            return;
        }
        Context context = activity.getApplicationContext();

        // A barcode detector is created to track barcodes. An associated multi-processor instance
        // is set to receive the barcode detection results, track the barcodes, and maintain
        // graphics for each barcode on screen. The factory is used by the multi-processor to
        // create a separate tracker instance for each barcode.
        BarcodeDetector barcodeDetector =
                new BarcodeDetector.Builder(context).setBarcodeFormats(Barcode.ALL_FORMATS).build();
        BarcodeTrackerFactory barcodeFactory = new BarcodeTrackerFactory(this);
        barcodeDetector.setProcessor(new MultiProcessor.Builder<>(barcodeFactory).build());

        if (!barcodeDetector.isOperational()) {
            // Note: The first time that an app using the barcode or face API is installed on a
            // device, GMS will download a native libraries to the device in order to do detection.
            // Usually this completes before the app is run for the first time. But if that
            // download has not yet completed, then the above call will not detect any barcodes.
            //
            // isOperational() can be used to check if the required native libraries are currently
            // available. The detectors will automatically become operational once the library
            // downloads complete on device.
            Log.w(TAG, "Detector dependencies are not yet available.");
        }

        // Creates and starts the camera. Note that this uses a higher resolution in comparison
        // to other detection examples to enable the barcode detector to detect small barcodes
        // at long distances.
        DisplayMetrics metrics = new DisplayMetrics();
        activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        CameraSource.Builder builder =
                new CameraSource.Builder(context, barcodeDetector)
                        .setFacing(CameraSource.CAMERA_FACING_BACK)
                        .setRequestedPreviewSize(metrics.widthPixels, metrics.heightPixels)
                        .setRequestedFps(24.0f);

        // Make sure that auto focus is an available option
        builder =
                builder.setFocusMode(
                        autoFocus ? Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE : null);

        mCameraSource =
                builder.setFlashMode(useFlash ? Camera.Parameters.FLASH_MODE_TORCH : null).build();
    }

    /**
     * Starts the camera source.
     *
     * @return true if the camera was started successfully, false otherwise.
     * @throws SecurityException if camera permission is not granted.
     */
    public boolean startCameraSource() throws SecurityException {
        if (mCameraSource == null || mCameraSourcePreview == null) {
            return false;
        }
        if (!mCameraSourcePreview.mCameraExist) {
            return false;
        }

        Activity activity = mHostProvider.getHostActivity();
        if (activity == null) {
            return false;
        }

        // Check that the device has play services available.
        try {
            int code =
                    GoogleApiAvailability.getInstance()
                            .isGooglePlayServicesAvailable(activity.getApplicationContext());
            if (code != ConnectionResult.SUCCESS) {
                Dialog dlg =
                        GoogleApiAvailability.getInstance()
                                .getErrorDialog(activity, code, RC_HANDLE_GMS);
                if (mCallback.onPlayServicesUnavailable(dlg)) {
                    return false;
                }
                if (dlg != null) {
                    dlg.show();
                }
            }
        } catch (ActivityNotFoundException e) {
            Log.e(TAG, "Unable to start camera source.", e);
            mCameraSource.release();
            mCameraSource = null;
            return false;
        }

        try {
            mCameraSourcePreview.start(mCameraSource);
            return true;
        } catch (IOException e) {
            Log.e(TAG, "Unable to start camera source.", e);
            mCameraSource.release();
            mCameraSource = null;
            return false;
        }
    }

    /** Stops the camera source preview. */
    public void stopCameraSource() {
        if (mCameraSourcePreview != null) {
            mCameraSourcePreview.stop();
        }
    }

    /** Releases all camera resources. Should be called in onDestroy. */
    public void release() {
        if (mDisplayManager != null) {
            mDisplayManager.unregisterDisplayListener(mDisplayListener);
        }
        if (mCameraSourcePreview != null) {
            mCameraSourcePreview.release();
        }
        mCameraSource = null;
    }

    /**
     * Handles configuration changes (orientation). Should be called in onConfigurationChanged.
     *
     * @return true if configuration change was handled, false if host is invalid.
     */
    public boolean handleConfigurationChange() {
        if (!mHostProvider.isHostValid()) {
            return false;
        }

        if (mCameraSourcePreview != null) {
            mCameraSourcePreview.stop();
            try {
                startCameraSource();
            } catch (SecurityException exc) {
                Log.e(TAG, "Security exception when restarting camera source", exc);
            }
        }
        return true;
    }

    /**
     * Checks if the rotation change represents a 180-degree rotation. 180-degree rotations are:
     * ROTATION_0 <-> ROTATION_180, ROTATION_90 <-> ROTATION_270
     */
    private boolean is180DegreeRotation(int oldRotation, int newRotation) {
        return (oldRotation == Surface.ROTATION_0 && newRotation == Surface.ROTATION_180)
                || (oldRotation == Surface.ROTATION_180 && newRotation == Surface.ROTATION_0)
                || (oldRotation == Surface.ROTATION_90 && newRotation == Surface.ROTATION_270)
                || (oldRotation == Surface.ROTATION_270 && newRotation == Surface.ROTATION_90);
    }

    /** Recreates and restarts the camera source to handle 180-degree rotation. */
    private void recreateCameraSource() {
        if (mCameraSourcePreview == null) {
            return;
        }

        mCameraSourcePreview.stop();
        if (mCameraSource != null) {
            mCameraSource.release();
            mCameraSource = null;
        }

        createCameraSource(true, false);
        try {
            startCameraSource();
        } catch (SecurityException exc) {
            Log.e(TAG, "Security exception when recreating camera source", exc);
        }
    }

    @Override
    public void onDetectedQrCode(Barcode barcode) {
        if (mCallback != null) {
            mCallback.onDetectedQrCode(barcode);
        }
    }

    /** Returns whether the camera source preview is available and has a camera. */
    public boolean isCameraAvailable() {
        return mCameraSourcePreview != null && mCameraSourcePreview.mCameraExist;
    }

    /** Returns the camera source preview. */
    public @Nullable CameraSourcePreview getCameraSourcePreview() {
        return mCameraSourcePreview;
    }
}
