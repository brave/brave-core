/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.model;

public class SponsoredLogo {
    private final String mImageUrl;
    private final String mAlt;
    private final String mCompanyName;
    private final String mDestinationUrl;

    public SponsoredLogo(String imageUrl, String alt, String companyName, String destinationUrl) {
        mImageUrl = imageUrl;
        mAlt = alt;
        mCompanyName = companyName;
        mDestinationUrl = destinationUrl;
    }

    public String getImageUrl() {
        return mImageUrl;
    }

    public String getAlt() {
        return mAlt;
    }

    public String getCompanyName() {
        return mCompanyName;
    }

    public String getDestinationUrl() {
        return mDestinationUrl;
    }
}
