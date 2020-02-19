/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp.sponsored;

public class SponsoredImage extends NTPImage {

    private String imageUrl;
    private int focalPointX;
    private int focalPointY;

    public SponsoredImage(String imageUrl, int focalPointX, int focalPointY) {
        this.imageUrl = imageUrl;
        this.focalPointX = focalPointX;
        this.focalPointY = focalPointY;
    }

    public String getImageUrl() {
        return imageUrl;
    }

    public int getFocalPointX() {
        return focalPointX;
    }

    public int getFocalPointY() {
        return focalPointY;
    }
}