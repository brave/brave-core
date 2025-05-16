/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.model;

public class SponsoredLogo {
    private final String imageUrl;
    private final String alt;
    private final String companyName;
    private final String destinationUrl;

    public SponsoredLogo(String imageUrl, String alt, String companyName, String destinationUrl) {
        this.imageUrl = imageUrl;
        this.alt = alt;
        this.companyName = companyName;
        this.destinationUrl = destinationUrl;
    }

    public String getImageUrl() {
        return imageUrl;
    }

    public String getAlt() {
        return alt;
    }

    public String getCompanyName() {
        return companyName;
    }

    public String getDestinationUrl() {
        return destinationUrl;
    }
}
