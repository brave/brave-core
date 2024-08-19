/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.models;

import androidx.annotation.NonNull;

public class BraveVpnServerRegion {
    private String mCountryIsoCode;
    private String mRegionName;
    private String mRegionNamePretty;
    private String mRegionCityNamePretty;

    public BraveVpnServerRegion(
            String countryIsoCode,
            String regionName,
            String regionNamePretty,
            String regionCityNamePretty) {
        this.mCountryIsoCode = countryIsoCode;
        this.mRegionName = regionName;
        this.mRegionNamePretty = regionNamePretty;
        this.mRegionCityNamePretty = regionCityNamePretty;
    }

    public String getCountryIsoCode() {
        return mCountryIsoCode;
    }

    public String getRegionName() {
        return mRegionName;
    }

    public String getRegionNamePretty() {
        return mRegionNamePretty;
    }

    public String getRegionCityNamePretty() {
        return mRegionCityNamePretty;
    }

    @NonNull
    @Override
    public String toString() {
        return "BraveVpnServerRegion{"
                + "mCountryIsoCode='"
                + mCountryIsoCode
                + '\''
                + ", mRegionName='"
                + mRegionName
                + '\''
                + ", mRegionNamePretty='"
                + mRegionNamePretty
                + '\''
                + ", mRegionCityNamePretty='"
                + mRegionCityNamePretty
                + '\''
                + '}';
    }
}
