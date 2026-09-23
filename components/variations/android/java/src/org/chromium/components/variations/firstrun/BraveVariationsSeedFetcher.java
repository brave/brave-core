/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.variations.firstrun;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.CommandLine;
import org.chromium.base.Log;
import org.chromium.components.variations.VariationsSwitches;

public class BraveVariationsSeedFetcher extends VariationsSeedFetcher {
    // To delete in bytecode. Variables from the parent class will be used instead.
    private static Object sLock;
    private static String DEFAULT_VARIATIONS_SERVER_URL;
    private static String DEFAULT_FAST_VARIATIONS_SERVER_URL;

    private static VariationsSeedFetcher sInstance;
    private static final String TAG = "BraveVariations";

    public static VariationsSeedFetcher get() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new BraveVariationsSeedFetcher();
            }
            return sInstance;
        }
    }

    @VisibleForTesting
    public BraveVariationsSeedFetcher() {
        super();
    }

    @VisibleForTesting
    @Override
    protected String getConnectionString(SeedFetchParameters params) {
        String urlString = super.getConnectionString(params);

        // Fix channel name.
        urlString = urlString.replaceAll("=canary", "=nightly");

        // Return as is if URL was passed manually.
        if (CommandLine.getInstance().hasSwitch(VariationsSwitches.VARIATIONS_SERVER_URL)) {
            return urlString;
        }

        // Replace Chromium URL with Brave URL.
        if (urlString.indexOf(DEFAULT_VARIATIONS_SERVER_URL) != -1) {
            urlString = urlString.replaceFirst(
                    DEFAULT_VARIATIONS_SERVER_URL, BraveVariationsConfig.VARIATIONS_SERVER_URL);
        } else if (urlString.indexOf(DEFAULT_FAST_VARIATIONS_SERVER_URL) != -1) {
            urlString = urlString.replaceFirst(DEFAULT_FAST_VARIATIONS_SERVER_URL,
                    BraveVariationsConfig.VARIATIONS_SERVER_URL);
        } else {
            Log.e(TAG, "Cannot replace seed URL to fetch variations: %s", urlString);
            urlString = "";
        }

        Log.i(TAG, "Fetching variations from: %s", urlString);
        return urlString;
    }
}
