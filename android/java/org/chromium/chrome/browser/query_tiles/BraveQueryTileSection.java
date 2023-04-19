/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.query_tiles;

import android.content.Context;

import org.chromium.base.Log;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.user_prefs.UserPrefs;

public class BraveQueryTileSection {
    private static final String TAG = "BraveQTSection";

    public static int getMaxRowsForMostVisitedTiles(Context context) {
        try {
            if (!ProfileManager.isInitialized()
                    || !UserPrefs.get(BraveActivity.getBraveActivity().getCurrentProfile())
                                .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)) {
                return 2;
            } else {
                return 1;
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "getMaxRowsForMostVisitedTiles ", e);
        }

        return 2;
    }
}
