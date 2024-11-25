/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.feedback.ScreenshotTask;

import java.io.ByteArrayOutputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BraveShieldsScreenshotUtil {
    private static final String TAG = "ShieldsScreenshot";
    private final BraveScreenshotRunnable mBraveScreenshotRunnable;

    private static class PngConvertorTask implements Runnable {
        private static final int PNG_QUALITY = 100;
        private Bitmap mBitmap;
        private final BraveShieldsScreenshotUtilCallback mCallback;

        public PngConvertorTask(
                @Nullable Bitmap bitmap, @NonNull BraveShieldsScreenshotUtilCallback callback) {
            mCallback = callback;
            mBitmap = bitmap;
        }

        @Override
        public void run() {
            assert !ThreadUtils.runningOnUiThread();

            try {
                if (mBitmap == null) {
                    ThreadUtils.postOnUiThread(
                            () -> {
                                onPostExecute(null);
                            });
                    return;
                }

                ByteArrayOutputStream bos = new ByteArrayOutputStream();
                mBitmap.compress(CompressFormat.PNG, PNG_QUALITY, bos);

                ThreadUtils.postOnUiThread(
                        () -> {
                            onPostExecute(bos.toByteArray());
                        });
            } catch (Exception ex) {
                ThreadUtils.postOnUiThread(
                        () -> {
                            onPostExecute(null);
                        });
            }
        }

        private void onPostExecute(byte[] pngBytes) {
            assert ThreadUtils.runningOnUiThread();
            mCallback.onScreenshotReady(pngBytes);
        }
    }

    private static class BraveScreenshotRunnable implements Runnable {
        private static final int TIMEOUT_MS = 500;
        private static final int RETRY_COUNT = 5;
        private int mRetryCounter;
        private final BraveShieldsScreenshotUtilCallback mCallback;
        private final ScreenshotTask mScreenshotTask;

        public BraveScreenshotRunnable(
                @NonNull ScreenshotTask screenshotTask,
                @NonNull BraveShieldsScreenshotUtilCallback callback) {
            mCallback = callback;
            mScreenshotTask = screenshotTask;
        }

        private void start() {
            mScreenshotTask.capture(this);
        }

        @Override
        public void run() {
            assert ThreadUtils.runningOnUiThread();

            if (!isReady()) {
                ThreadUtils.postOnUiThreadDelayed(this, TIMEOUT_MS);
                mRetryCounter++;
                return;
            }

            if (isRetryCounterOver()) {
                mCallback.onScreenshotReady(null);
                return;
            }

            ExecutorService es = Executors.newSingleThreadExecutor();
            es.execute(new PngConvertorTask(mScreenshotTask.getScreenshot(), mCallback));
        }

        private boolean isRetryCounterOver() {
            return mRetryCounter >= RETRY_COUNT;
        }

        private boolean isReady() {
            return mScreenshotTask.isReady() && !isRetryCounterOver();
        }
    }

    public interface BraveShieldsScreenshotUtilCallback {
        void onScreenshotReady(@Nullable byte[] pngBytes);
    }

    public BraveShieldsScreenshotUtil(
            @NonNull Context context, @NonNull BraveShieldsScreenshotUtilCallback callback) {
        mBraveScreenshotRunnable =
                new BraveScreenshotRunnable(new ScreenshotTask((Activity) context), callback);
    }

    public void capture() {
        mBraveScreenshotRunnable.start();
    }
}
