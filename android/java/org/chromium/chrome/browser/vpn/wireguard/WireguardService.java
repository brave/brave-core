/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.wireguard;

import org.chromium.build.annotations.IdentifierNameString;
import org.chromium.chrome.browser.base.SplitCompatService;

/** See {@link WireguardServiceImpl}. */
public class WireguardService extends SplitCompatService {
    @IdentifierNameString
    private static String sImplClassName =
            "org.chromium.chrome.browser.vpn.wireguard.WireguardServiceImpl";

    public WireguardService() {
        super(sImplClassName);
    }
}