/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.signin;

import android.accounts.Account;

import androidx.annotation.MainThread;
import androidx.annotation.Nullable;

import org.chromium.base.Callback;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.AppHooks;
import org.chromium.chrome.browser.signin.services.SigninManager;
import org.chromium.components.signin.base.CoreAccountId;
import org.chromium.components.signin.base.CoreAccountInfo;
import org.chromium.components.signin.identitymanager.AccountInfoServiceProvider;
import org.chromium.components.signin.identitymanager.AccountTrackerService;
import org.chromium.components.signin.identitymanager.IdentityManager;
import org.chromium.components.signin.identitymanager.IdentityMutator;
import org.chromium.components.signin.metrics.SigninAccessPoint;
import org.chromium.components.signin.metrics.SignoutReason;

public class BraveSigninManager implements SigninManager {
    private final IdentityManager mIdentityManager;

    BraveSigninManager(IdentityManager identityManager) {
        mIdentityManager = identityManager;
    }

    @Override
    public boolean isSigninAllowed() {
        return false;
    }

    @Override
    public boolean isSigninSupported() {
        return false;
    }

    @Override
    public void isAccountManaged(String email, final Callback<Boolean> callback) {}

    @Override
    public String getManagementDomain() {
        return "";
    }

    @Override
    public void signOut(@SignoutReason int signoutSource, SignOutCallback signOutCallback,
            boolean forceWipeUserData) {}

    @Override
    @MainThread
    public void runAfterOperationInProgress(Runnable runnable) {}

    @Override
    @Deprecated
    public void signinAndEnableSync(@SigninAccessPoint int accessPoint, Account account,
            @Nullable SignInCallback callback) {}

    @Override
    public void signin(Account account, @Nullable SignInCallback callback) {}

    @Override
    public void removeSignInStateObserver(SignInStateObserver observer) {}

    @Override
    public void addSignInStateObserver(SignInStateObserver observer) {}

    @Override
    public boolean isForceSigninEnabled() {
        return false;
    }

    @Override
    public boolean isSigninDisabledByPolicy() {
        return false;
    }

    @Override
    public void onFirstRunCheckDone() {}

    @Override
    public IdentityManager getIdentityManager() {
        return mIdentityManager;
    }

    @Override
    public String extractDomainName(String accountEmail) {
        return "";
    };

    @Override
    public void reloadAllAccountsFromSystem(@Nullable CoreAccountId primaryAccountId) {}

    @CalledByNative
    static SigninManager create(long nativeSigninManagerAndroid,
            AccountTrackerService accountTrackerService, IdentityManager identityManager,
            IdentityMutator identityMutator) {
        AccountInfoServiceProvider.init(identityManager, accountTrackerService);
        return new BraveSigninManager(identityManager);
    }

    @CalledByNative
    void destroy() {
        AccountInfoServiceProvider.get().destroy();
    }

    @Override
    public void wipeSyncUserData(Runnable wipeDataCallback) {}

    @Override
    public boolean isSyncOptInAllowed() {
        return false;
    }
}
