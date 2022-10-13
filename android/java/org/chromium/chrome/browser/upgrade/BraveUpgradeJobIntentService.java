/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.upgrade;

import org.chromium.build.annotations.IdentifierNameString;
import org.chromium.chrome.browser.base.SplitCompatJobIntentService;

public class BraveUpgradeJobIntentService extends SplitCompatJobIntentService {
    @IdentifierNameString
    private static String sImplClassName =
            "org.chromium.chrome.browser.upgrade.BraveUpgradeJobIntentServiceImpl";

    public BraveUpgradeJobIntentService() {
        super(sImplClassName);
    }
}
