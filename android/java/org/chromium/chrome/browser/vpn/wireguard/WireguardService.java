// Copyright 2022 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vpn.wireguard;

import org.chromium.base.annotations.IdentifierNameString;
import org.chromium.chrome.browser.base.SplitCompatService;

/** See {@link WireguardServiceImpl}. */
public class WireguardService extends SplitCompatService {
    @IdentifierNameString
    private static final String IMPL_CLASS_NAME =
            "org.chromium.chrome.browser.vpn.wireguard.WireguardServiceImpl";

    public WireguardService() {
        super(IMPL_CLASS_NAME);
    }
}