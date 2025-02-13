/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.model;

public class SponsoredRichMedia extends NTPImage {
    private String mWallpaperUrl;
    private String mCreativeInstanceId;
    private String mPlacementId;
    private String mTargetUrl;

    public SponsoredRichMedia(
            String wallpaperUrl, String creativeInstanceId, String placementId, String targetUrl) {
        mWallpaperUrl = wallpaperUrl;
        mCreativeInstanceId = creativeInstanceId;
        mPlacementId = placementId;
        mTargetUrl = targetUrl;
    }

    public String getWallpaperUrl() {
        return mWallpaperUrl;
    }

    public void setWallpaperUrl(String wallpaperUrl) {
        mWallpaperUrl = wallpaperUrl;
    }

    public String getCreativeInstanceId() {
        return mCreativeInstanceId;
    }

    public void setCreativeInstanceId(String creativeInstanceId) {
        mCreativeInstanceId = creativeInstanceId;
    }

    public String getPlacementId() {
        return mPlacementId;
    }

    public void setPlacementId(String placementId) {
        mPlacementId = placementId;
    }

    public String getTargetUrl() {
        return mTargetUrl;
    }

    public void setTargetUrl(String targetUrl) {
        mTargetUrl = targetUrl;
    }
}
