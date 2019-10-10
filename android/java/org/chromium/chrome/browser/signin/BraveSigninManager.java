/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.signin;

import android.content.Context;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.components.signin.AccountTrackerService;
import org.chromium.components.sync.AndroidSyncSettings;

public class BraveSigninManager extends SigninManager {
    BraveSigninManager(Context context, long nativeSigninManagerAndroid,
            AccountTrackerService accountTrackerService) {
        super(context, nativeSigninManagerAndroid, accountTrackerService,
                AndroidSyncSettings.get());
    }

    @Override
    public boolean isSignInAllowed() {
        return false;
    }

    @Override
    public boolean isSigninSupported() {
        return false;
    }

    @CalledByNative
    private static SigninManager create(
            long nativeSigninManagerAndroid, AccountTrackerService accountTrackerService) {
        assert nativeSigninManagerAndroid != 0;
        assert accountTrackerService != null;
        return new BraveSigninManager(ContextUtils.getApplicationContext(),
                nativeSigninManagerAndroid, accountTrackerService);
    }
}
