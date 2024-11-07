/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base.shared_preferences;

import org.chromium.base.BravePreferenceKeys;

public class BraveStrictPreferenceKeyChecker extends StrictPreferenceKeyChecker {
    BraveStrictPreferenceKeyChecker(PreferenceKeyRegistry registry) {
        super(registry);
    }

    @Override
    public void checkIsKeyInUse(String key) {
        if (!isKeyInUse(key) && !BravePreferenceKeys.isBraveKeyInUse(key)) {
            // Do nothing here for now. We might reconsider it in the future
            // in case we want to make some kind of sanitation of pref keys
            // used in Brave.
            // Upstreams function isKeyInUse is called here and Brave's
            // BravePreferenceKeys.isBraveKeyInUse is just a stub method
        }
    }

    /**
     * Dummy method that will be deleted form the bytecode. {@link
     * StrictPreferenceKeyChecker#isKeyInUse} will be used instead.
     */
    private boolean isKeyInUse(String unused_key) {
        assert false : "This method should be deleted in bytecode!";
        return false;
    }
}
