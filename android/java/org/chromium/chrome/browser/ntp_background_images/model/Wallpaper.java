/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.model;

public class Wallpaper extends NTPImage {
    private final String mImagePath;
    private final int mFocalPointX;
    private final int mFocalPointY;
    private final String mLogoPath;
    private final String mLogoDestinationUrl;
    private final String mThemeName;
    private final boolean mIsSponsored;
    private final String mCreativeInstanceId;
    private final String mWallpaperId;
    private final boolean mIsRichMedia;
    private final boolean mShouldMetricsFallbackToP3a;

    public Wallpaper(
            String imagePath,
            int focalPointX,
            int focalPointY,
            String logoPath,
            String logoDestinationUrl,
            String themeName,
            boolean isSponsored,
            String creativeInstanceId,
            String wallpaperId,
            boolean isRichMedia,
            boolean shouldMetricsFallbackToP3a) {
        mImagePath = imagePath;
        mFocalPointX = focalPointX;
        mFocalPointY = focalPointY;
        mLogoPath = logoPath;
        mLogoDestinationUrl = logoDestinationUrl;
        mThemeName = themeName;
        mIsSponsored = isSponsored;
        mCreativeInstanceId = creativeInstanceId;
        mWallpaperId = wallpaperId;
        mIsRichMedia = isRichMedia;
        mShouldMetricsFallbackToP3a = shouldMetricsFallbackToP3a;
    }

    public String getImagePath() {
        return mImagePath;
    }

    public int getFocalPointX() {
        return mFocalPointX;
    }

    public int getFocalPointY() {
        return mFocalPointY;
    }

    public String getLogoPath() {
        return mLogoPath;
    }

    public String getLogoDestinationUrl() {
        return mLogoDestinationUrl;
    }

    public String getThemeName() {
        return mThemeName;
    }

    public boolean isSponsored() {
        return mIsSponsored;
    }

    public String getCreativeInstanceId() {
        return mCreativeInstanceId;
    }

    public String getWallpaperId() {
        return mWallpaperId;
    }

    public boolean isRichMedia() {
        return mIsRichMedia;
    }

    public boolean shouldMetricsFallbackToP3a() {
        return mShouldMetricsFallbackToP3a;
    }
}
