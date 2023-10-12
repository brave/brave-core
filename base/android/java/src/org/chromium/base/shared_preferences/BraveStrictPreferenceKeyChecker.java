/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base.shared_preferences;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Log;

public class BraveStrictPreferenceKeyChecker extends StrictPreferenceKeyChecker {
    private static String TAG = "BraveReflectionUtil";

    BraveStrictPreferenceKeyChecker(PreferenceKeyRegistry registry) {
        super(registry);
    }

    @Override
    public void checkIsKeyInUse(String key) {
        if (!isKeyInUse(key) && !BravePreferenceKeys.isBraveKeyInUse(key)) {
            Log.e(
                    TAG,
                    "Key "
                            + key
                            + " is not registered for Brave's use. Either add it to"
                            + " BravePreferenceKeys or remove it if it's not used.");
        }
    }

    /*
     * Dummy method that will be deleted form the bytecode. {@link
     * StrictPreferenceKeyChecker#isKeyInUse} will be used instead.
     */
    private boolean isKeyInUse(String key) {
        assert false : "This method should be deleted in bytecode!";
        return false;
    }
}
