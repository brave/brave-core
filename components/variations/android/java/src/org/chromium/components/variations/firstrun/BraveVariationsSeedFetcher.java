/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.variations.firstrun;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.CommandLine;
import org.chromium.components.variations.VariationsSwitches;

public class BraveVariationsSeedFetcher extends VariationsSeedFetcher {
    private static VariationsSeedFetcher sInstance;

    public static VariationsSeedFetcher get() {
        if (sInstance == null) {
            sInstance = new BraveVariationsSeedFetcher();
        }
        return sInstance;
    }

    @VisibleForTesting
    public BraveVariationsSeedFetcher() {
        super();
    }

    @VisibleForTesting
    @Override
    protected String getConnectionString(SeedFetchParameters params) {
        if (!CommandLine.getInstance().hasSwitch(VariationsSwitches.VARIATIONS_SERVER_URL)) {
            // If there is no alternative variations server specified, we still don't want to
            // connect to Google's server.
            return "";
        }

        String urlString = super.getConnectionString(params);
        // Relacing Google's variations server with ours, but keep parameters.
        int paramsPosition = urlString.indexOf("?", 0);
        urlString =
                CommandLine.getInstance().getSwitchValue(VariationsSwitches.VARIATIONS_SERVER_URL)
                + (paramsPosition > 0 ? urlString.substring(paramsPosition) : "");
        return urlString;
    }
}
