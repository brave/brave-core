/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.model;

public class BackgroundImage extends NTPImage {
    private final int mImageDrawable;
    private final int mCenterPointX;
    private final int mCenterPointY;
    private String mImagePath;
    private ImageCredit mImageCredit;

    public BackgroundImage(
            int imageDrawable, int centerPointX, int centerPointY, ImageCredit imageCredit) {
        mImageDrawable = imageDrawable;
        mCenterPointX = centerPointX;
        mCenterPointY = centerPointY;
        mImageCredit = imageCredit;
        mImagePath = null;
    }

    public BackgroundImage(
            String imagePath, int centerPointX, int centerPointY, ImageCredit imageCredit) {
        mImageDrawable = 0;
        mCenterPointX = centerPointX;
        mCenterPointY = centerPointY;
        mImageCredit = imageCredit;
        mImagePath = imagePath;
    }

    public BackgroundImage(int imageDrawable, int centerPointX, int centerPointY) {
        mImageDrawable = imageDrawable;
        mCenterPointX = centerPointX;
        mCenterPointY = centerPointY;
    }

    public int getImageDrawable() {
        return mImageDrawable;
    }

    public String getImagePath() {
        return mImagePath;
    }

    public int getCenterPointX() {
        return mCenterPointX;
    }

    public int getCenterPointY() {
        return mCenterPointY;
    }

    public ImageCredit getImageCredit() {
        return mImageCredit;
    }
}
