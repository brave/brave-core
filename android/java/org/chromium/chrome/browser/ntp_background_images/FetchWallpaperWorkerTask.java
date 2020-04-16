/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.Pair;
import android.net.Uri;
import java.io.InputStream;
import java.io.FileNotFoundException;
import android.graphics.BitmapFactory;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.browser.ntp_background_images.NTPImage;

public class FetchWallpaperWorkerTask extends AsyncTask<Pair<Bitmap, Bitmap>> {
    public interface WallpaperRetrievedCallback {
        void bgWallpaperRetrieved(Bitmap bgWallpaper);
        void logoRetrieved(NTPBackgroundImagesBridge.Wallpaper mWallpaper, Bitmap logoWallpaper);
    }

    private Context mContext;
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
        mContext = ContextUtils.getApplicationContext();
    }

    @Override
    protected Pair<Bitmap, Bitmap> doInBackground() {
        Bitmap logoBitmap = null;
        if (mNTPImage instanceof NTPBackgroundImagesBridge.Wallpaper) {
            NTPBackgroundImagesBridge.Wallpaper mWallpaper = (NTPBackgroundImagesBridge.Wallpaper) mNTPImage;
            if (mWallpaper.getLogoPath() != null ) {
                try {
                    Uri logoFileUri = Uri.parse("file://"+ mWallpaper.getLogoPath());
                    InputStream inputStream = mContext.getContentResolver().openInputStream(logoFileUri);
                    logoBitmap = BitmapFactory.decodeStream(inputStream);
                } catch(FileNotFoundException exc) {
                    Log.e("NTP", exc.getMessage());
                }
            }
        }        

        return new Pair<Bitmap, Bitmap>(
            NTPUtil.getWallpaperBitmap(mNTPImage, mLayoutWidth, mLayoutHeight), 
            logoBitmap);
    }

    @Override
    protected void onPostExecute(Pair<Bitmap, Bitmap> wallpapers) {
        assert ThreadUtils.runningOnUiThread();

        if (isCancelled()) return;

        if (wallpapers.first != null && !wallpapers.first.isRecycled())
            mCallback.bgWallpaperRetrieved(wallpapers.first);

        if (wallpapers.second != null && !wallpapers.second.isRecycled())
            mCallback.logoRetrieved((NTPBackgroundImagesBridge.Wallpaper) mNTPImage, wallpapers.second);
    }
}
