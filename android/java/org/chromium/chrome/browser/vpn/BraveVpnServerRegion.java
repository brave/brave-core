/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn;

public class BraveVpnServerRegion {
    private String continent;
    private String name;
    private String namePretty;

    public BraveVpnServerRegion(String continent, String name, String namePretty) {
        this.continent = continent;
        this.name = name;
        this.namePretty = namePretty;
    }

    public String getContinent() {
        return continent;
    }

    public String getName() {
        return name;
    }

    public String getNamePretty() {
        return namePretty;
    }
}
