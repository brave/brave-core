/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.default_browser_promo;

import android.app.Activity;

import org.chromium.components.feature_engagement.Tracker;
import org.chromium.ui.base.WindowAndroid;

/** A utility class providing information regarding states of default browser. */
public class BraveDefaultBrowserPromoUtils extends DefaultBrowserPromoUtils {

    public BraveDefaultBrowserPromoUtils(
            DefaultBrowserPromoImpressionCounter impressionCounter,
            DefaultBrowserStateProvider stateProvider) {
        super(impressionCounter, stateProvider);
    }

    @Override
    public boolean prepareLaunchPromoIfNeeded(
            Activity activity,
            WindowAndroid windowAndroid,
            Tracker tracker,
            boolean ignoreMaxCount) {
        return false;
    }
}
