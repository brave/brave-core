/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.signin;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.AppHooks;
import org.chromium.chrome.browser.signin.services.SigninManager;
import org.chromium.chrome.browser.sync.AndroidSyncSettings;
import org.chromium.components.externalauth.ExternalAuthUtils;
import org.chromium.components.signin.AccountTrackerService;
import org.chromium.components.signin.identitymanager.IdentityManager;
import org.chromium.components.signin.identitymanager.IdentityMutator;

public class BraveSigninManager extends SigninManagerImpl {
    BraveSigninManager(long nativeSigninManagerAndroid, AccountTrackerService accountTrackerService,
            IdentityManager identityManager, IdentityMutator identityMutator,
            AndroidSyncSettings androidSyncSettings, ExternalAuthUtils externalAuthUtils) {
        super(nativeSigninManagerAndroid, accountTrackerService, identityManager, identityMutator,
                androidSyncSettings, externalAuthUtils);
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
    private static SigninManager create(long nativeSigninManagerAndroid,
            AccountTrackerService accountTrackerService, IdentityManager identityManager,
            IdentityMutator identityMutator) {
        assert nativeSigninManagerAndroid != 0;
        assert accountTrackerService != null;
        assert identityManager != null;
        assert identityMutator != null;
        return new BraveSigninManager(nativeSigninManagerAndroid, accountTrackerService,
                identityManager, identityMutator, AndroidSyncSettings.get(),
                ExternalAuthUtils.getInstance());
    }
}
