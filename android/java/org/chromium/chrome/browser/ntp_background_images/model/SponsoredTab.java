/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.model;

import org.chromium.base.Callback;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.util.NTPImageUtil;
import org.chromium.chrome.browser.ntp_background_images.util.SponsoredImageUtil;

public class SponsoredTab {
    private final NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;
    private NTPImage mNtpImage;
    private boolean mNTPImageReady;

    public SponsoredTab(
            NTPBackgroundImagesBridge mNTPBackgroundImagesBridge, boolean allowSponsoredImage) {
        this.mNTPBackgroundImagesBridge = mNTPBackgroundImagesBridge;
    }

    public void getNTPImage(boolean allowSponsoredImage, Callback<NTPImage> callback) {
        // Return cached NTP image if available. We maintain only one NTP image per tab.
        if (mNTPImageReady) {
            callback.onResult(mNtpImage);
            return;
        }

        NTPImageUtil.getNTPImage(
                mNTPBackgroundImagesBridge,
                allowSponsoredImage,
                ntpImage -> getNTPImageCallback(ntpImage, callback));
    }

    private void getNTPImageCallback(NTPImage ntpImage, Callback<NTPImage> callback) {
        mNtpImage = ntpImage;

        if (mNtpImage == null) {
            mNtpImage = SponsoredImageUtil.getBackgroundImage();
        } else if (mNtpImage instanceof Wallpaper) {
            Wallpaper wallpaper = (Wallpaper) mNtpImage;
            if (wallpaper == null) {
                mNtpImage = SponsoredImageUtil.getBackgroundImage();
            }
        }

        mNTPImageReady = true;
        callback.onResult(mNtpImage);
    }
}
