/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.models;

import android.os.Parcel;
import android.os.Parcelable;

public class BraveVpnServerRegion implements Parcelable {
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

    // Getters for your member variables

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

    // Parcelable implementation

    protected BraveVpnServerRegion(Parcel in) {
        mContinent = in.readString();
        mCountryIsoCode = in.readString();
        mName = in.readString();
        mNamePretty = in.readString();
    }

    public static final Creator<BraveVpnServerRegion> CREATOR = new Creator<BraveVpnServerRegion>() {
        @Override
        public BraveVpnServerRegion createFromParcel(Parcel in) {
            return new BraveVpnServerRegion(in);
        }

        @Override
        public BraveVpnServerRegion[] newArray(int size) {
            return new BraveVpnServerRegion[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(mContinent);
        dest.writeString(mCountryIsoCode);
        dest.writeString(mName);
        dest.writeString(mNamePretty);
    }
}
