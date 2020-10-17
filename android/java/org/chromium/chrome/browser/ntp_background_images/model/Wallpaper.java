/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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
    private String mCreativeSetId;
    private String mCampaignId;
    private String mAdvertiserId;

    public Wallpaper(String imagePath, int focalPointX, int focalPointY,
        String logoPath, String logoDestinationUrl , String themeName,
        boolean isSponsored, String creativeInstanceId, String creativeSetId,
        String campaignId, String advertiserId) {
        mImagePath = imagePath;
        mFocalPointX = focalPointX;
        mFocalPointY = focalPointY;
        mLogoPath = logoPath;
        mLogoDestinationUrl = logoDestinationUrl;
        mThemeName = themeName;
        mIsSponsored = isSponsored;
        mCreativeInstanceId = creativeInstanceId;
        mCreativeSetId = creativeSetId;
        mCampaignId = campaignId;
        mAdvertiserId = advertiserId;
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

    public String getCreativeSetId() {
        return mCreativeSetId;
    }

    public String getCampaignId() {
        return mCampaignId;
    }

    public String getAdvertiserId() {
        return mAdvertiserId;
    }
}
