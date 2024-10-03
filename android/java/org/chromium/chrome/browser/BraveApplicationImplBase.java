/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser;

import com.wireguard.android.backend.GoBackend;

import org.chromium.chrome.browser.base.SplitCompatApplication;
import org.chromium.chrome.browser.vpn.utils.BraveVpnProfileUtils;

// TODO(alexeybarabash): needs to be redone for cr130
// import org.chromium.components.safe_browsing.BraveSafeBrowsingApiHandler;

public class BraveApplicationImplBase extends SplitCompatApplication.Impl {
    @Override
    public void onCreate() {
        super.onCreate();
        if (SplitCompatApplication.isBrowserProcess()) {
            GoBackend.setAlwaysOnCallback(
                    new GoBackend.AlwaysOnCallback() {
                        @Override
                        public void alwaysOnTriggered() {
                            BraveVpnProfileUtils.getInstance().startVpn(getApplication());
                        }
                    });
            // Set a handler for SafeBrowsing. It has to be done only once for a process lifetime.
            // TODO(alexeybarabash): needs to be redone for cr130
            // SafeBrowsingApiBridge.setSafetyNetApiHandler(
            //              BraveSafeBrowsingApiHandler.getInstance());
        }
    }
}
