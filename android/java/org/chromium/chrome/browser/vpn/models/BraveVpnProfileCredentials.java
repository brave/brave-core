/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.models;

public class BraveVpnProfileCredentials {
    private final String mApiAuthToken;
    private final String mUsername;
    private final String mPassword;

    public BraveVpnProfileCredentials(String apiAuthToken, String username, String password) {
        this.mApiAuthToken = apiAuthToken;
        this.mUsername = username;
        this.mPassword = password;
    }

    public String getApiAuthToken() {
        return mApiAuthToken;
    }

    public String getUsername() {
        return mUsername;
    }

    public String getPassword() {
        return mPassword;
    }
}
