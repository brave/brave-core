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
    private String mRegionCityName;
    private String mRegionCityNamePretty;
    private String mRegionPrecision;

    public BraveVpnServerRegion(
            String countryIsoCode,
            String regionName,
            String regionNamePretty,
            String regionCityName,
            String regionCityNamePretty,
            String regionPrecision) {
        this.mCountryIsoCode = countryIsoCode;
        this.mRegionName = regionName;
        this.mRegionNamePretty = regionNamePretty;
        this.mRegionCityName = regionCityName;
        this.mRegionCityNamePretty = regionCityNamePretty;
        this.mRegionPrecision = regionPrecision;
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

    public String getRegionCityName() {
        return mRegionCityName;
    }

    public String getRegionCityNamePretty() {
        return mRegionCityNamePretty;
    }

    public String getRegionPrecision() {
        return mRegionPrecision;
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
                + ", mRegionCityName='"
                + mRegionCityName
                + '\''
                + ", mRegionCityNamePretty='"
                + mRegionCityNamePretty
                + '\''
                + '\''
                + ", mRegionPrecision='"
                + mRegionPrecision
                + '\''
                + '}';
    }
}
