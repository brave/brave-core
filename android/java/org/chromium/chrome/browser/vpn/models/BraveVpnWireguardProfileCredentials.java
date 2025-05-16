/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.models;

public class BraveVpnWireguardProfileCredentials {
    private final String mApiAuthToken;
    private final String mClientId;
    private final String mMappedIpv4Address;
    private final String mMappedIpv6Address;
    private final String mServerPublicKey;

    public BraveVpnWireguardProfileCredentials(String apiAuthToken, String clientId,
            String mappedIpv4Address, String mappedIpv6Address, String serverPublicKey) {
        this.mApiAuthToken = apiAuthToken;
        this.mClientId = clientId;
        this.mMappedIpv4Address = mappedIpv4Address;
        this.mMappedIpv6Address = mappedIpv6Address;
        this.mServerPublicKey = serverPublicKey;
    }

    public String getApiAuthToken() {
        return mApiAuthToken;
    }

    public String getClientId() {
        return mClientId;
    }

    public String getMappedIpv4Address() {
        return mMappedIpv4Address;
    }

    public String getMappedIpv6Address() {
        return mMappedIpv6Address;
    }

    public String getServerPublicKey() {
        return mServerPublicKey;
    }
}