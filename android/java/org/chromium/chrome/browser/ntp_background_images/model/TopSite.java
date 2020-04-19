/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.model;

public class TopSite {
    private String mName;
    private String mDestinationUrl;
    private String mBackgroundColor;
    private String mImagePath;

    public TopSite(String name, String destinationUrl, String backgroundColor, String imagePath) {
        mName = name;
        mDestinationUrl = destinationUrl;
        mBackgroundColor = backgroundColor;
        mImagePath = imagePath;
    }

    public String getName() {
        return mName;
    }        

    public String getDestinationUrl() {
        return mDestinationUrl;
    }

    public String getBackgroundColor() {
        return mBackgroundColor;
    }

    public String getImagePath() {
        return mImagePath;
    }
}