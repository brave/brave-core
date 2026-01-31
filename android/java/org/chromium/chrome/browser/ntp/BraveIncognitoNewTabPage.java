/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.app.Activity;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.ui.native_page.NativePageHost;
import org.chromium.chrome.browser.vpn.BraveVpnPolicy;

/** Brave's extension for IncognitoNewTabPage to add policy checks. */
@NullMarked
public class BraveIncognitoNewTabPage extends IncognitoNewTabPage {

    public BraveIncognitoNewTabPage(
            Activity activity,
            NativePageHost host,
            Tab tab,
            ObservableSupplier<EdgeToEdgeController> edgeToEdgeControllerSupplier,
            @Nullable IncognitoNtpMetrics incognitoNtpMetrics) {
        super(activity, host, tab, edgeToEdgeControllerSupplier, incognitoNtpMetrics);

        // Pass VPN policy state to the view for VPN CTA visibility
        mIncognitoNewTabPageView.setVpnDisabledByPolicy(
                BraveVpnPolicy.isDisabledByPolicy(tab.getProfile()));
    }
}
