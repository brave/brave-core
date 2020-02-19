/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp.sponsored;

public class SponsoredLogo {

    private String imageUrl;
    private String alt;
    private String companyName;
    private String destinationUrl;

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