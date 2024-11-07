/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.model;

public class Wallpaper extends NTPImage {
    private String mImagePath;
    private int mFocalPointX;
    private int mFocalPointY;
    private String mLogoPath;
    private String mLogoDestinationUrl;
    private String mThemeName;
    private boolean mIsSponsored;
    private String mCreativeInstanceId;
    private String mWallpaperId;

    public Wallpaper(String imagePath, int focalPointX, int focalPointY, String logoPath,
            String logoDestinationUrl, String themeName, boolean isSponsored,
            String creativeInstanceId, String wallpaperId) {
        mImagePath = imagePath;
        mFocalPointX = focalPointX;
        mFocalPointY = focalPointY;
        mLogoPath = logoPath;
        mLogoDestinationUrl = logoDestinationUrl;
        mThemeName = themeName;
        mIsSponsored = isSponsored;
        mCreativeInstanceId = creativeInstanceId;
        mWallpaperId = wallpaperId;
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
}
