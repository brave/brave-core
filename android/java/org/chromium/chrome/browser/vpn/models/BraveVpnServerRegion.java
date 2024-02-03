/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.models;

public class BraveVpnServerRegion {
    private String mContinent;
    private String mCountryIsoCode;
    private String mName;
    private String mNamePretty;

    public BraveVpnServerRegion(
            String continent, String countryIsoCode, String name, String namePretty) {
        this.mContinent = continent;
        this.mCountryIsoCode = countryIsoCode;
        this.mName = name;
        this.mNamePretty = namePretty;
    }

    public String getContinent() {
        return mContinent;
    }

    public String getCountryIsoCode() {
        return mCountryIsoCode;
    }

    public String getName() {
        return mName;
    }

    public String getNamePretty() {
        return mNamePretty;
    }
}
