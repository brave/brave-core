/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images;

import android.graphics.Bitmap;

import org.chromium.base.ThreadUtils;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.browser.ntp_background_images.NTPImage;

public class FetchWallpaperWorkerTask extends AsyncTask<Bitmap> {
    public interface WallpaperRetrievedCallback {
        void wallpaperRetrieved(Bitmap wallpaperBitmap);
    }

    private NTPImage mNTPImage;
    private int mLayoutWidth;
    private int mLayoutHeight;

    // The callback to use to communicate the results.
    private WallpaperRetrievedCallback mCallback;

    public FetchWallpaperWorkerTask(NTPImage ntpImage, int layoutWidth, int layoutHeight,WallpaperRetrievedCallback callback) {
        mNTPImage = ntpImage;
        mLayoutWidth = layoutWidth;
        mLayoutHeight = layoutHeight;
        mCallback = callback;
    }

    @Override
    protected Bitmap doInBackground() {
        return NTPUtil.getWallpaperBitmap(mNTPImage, mLayoutWidth, mLayoutHeight);
    }

    @Override
    protected void onPostExecute(Bitmap wallpaper) {
        assert ThreadUtils.runningOnUiThread();

        if (isCancelled()) return;

        mCallback.wallpaperRetrieved(wallpaper);
    }
}
