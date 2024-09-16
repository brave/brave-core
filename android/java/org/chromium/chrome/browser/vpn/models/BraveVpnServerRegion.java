/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.models;

import androidx.annotation.NonNull;

public class BraveVpnServerRegion {
    private final boolean mIsAutoSelected;
    private final String mCountry;
    private final String mContinent;
    private final String mCountryIsoCode;
    private final String mRegionName;
    private final String mRegionNamePretty;
    private final String mRegionPrecision;

    public BraveVpnServerRegion(
            boolean isAutoSelected,
            String country,
            String continent,
            String countryIsoCode,
            String regionName,
            String regionNamePretty,
            String regionPrecision) {
        this.mIsAutoSelected = isAutoSelected;
        this.mCountry = country;
        this.mContinent = continent;
        this.mCountryIsoCode = countryIsoCode;
        this.mRegionName = regionName;
        this.mRegionNamePretty = regionNamePretty;
        this.mRegionPrecision = regionPrecision;
    }

    public boolean isAutoSelected() {
        return mIsAutoSelected;
    }

    public String getCountry() {
        return mCountry;
    }

    public String getContinent() {
        return mContinent;
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

    public String getRegionPrecision() {
        return mRegionPrecision;
    }

    @NonNull
    @Override
    public String toString() {
        return "BraveVpnServerRegion{"
                + "mIsAutoSelected="
                + mIsAutoSelected
                + ", mCountry='"
                + mCountry
                + '\''
                + ", mContinent='"
                + mContinent
                + '\''
                + ", mCountryIsoCode='"
                + mCountryIsoCode
                + '\''
                + ", mRegionName='"
                + mRegionName
                + '\''
                + ", mRegionNamePretty='"
                + mRegionNamePretty
                + '\''
                + ", mRegionPrecision='"
                + mRegionPrecision
                + '\''
                + '}';
    }
}
