/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.signin;

import androidx.annotation.MainThread;

import org.jni_zero.CalledByNative;
import org.jni_zero.JniType;

import org.chromium.base.Callback;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.signin.services.SigninManager;
import org.chromium.components.signin.base.CoreAccountInfo;
import org.chromium.components.signin.identitymanager.AccountInfoServiceProvider;
import org.chromium.components.signin.identitymanager.IdentityManager;
import org.chromium.components.signin.identitymanager.IdentityMutator;
import org.chromium.components.signin.metrics.SigninAccessPoint;
import org.chromium.components.signin.metrics.SignoutReason;

@NullMarked
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
    public boolean isSignOutAllowed() {
        return false;
    }

    @Override
    public boolean isSigninSupported(boolean requireUpdatedPlayServices) {
        return false;
    }

    @Override
    public void isAccountManaged(String email, final Callback<Boolean> callback) {}

    @Override
    public String getManagementDomain() {
        return "";
    }

    @Override
    public void signOut(
            @SignoutReason int signoutSource,
            @Nullable SignOutCallback signOutCallback,
            boolean forceWipeUserData) {}

    @Override
    @MainThread
    public void runAfterOperationInProgress(Runnable runnable) {}

    @Override
    public void removeSignInStateObserver(SignInStateObserver observer) {}

    @Override
    public void addSignInStateObserver(SignInStateObserver observer) {}

    @Override
    public boolean isForceSigninEnabled() {
        return false;
    }

    @Override
    public IdentityManager getIdentityManager() {
        return mIdentityManager;
    }

    @Override
    public String extractDomainName(String accountEmail) {
        return "";
    }

    @CalledByNative
    static SigninManager create(
            long nativeSigninManagerAndroid,
            @JniType("Profile*") Profile profile,
            @JniType("signin::IdentityManager*") IdentityManager identityManager,
            IdentityMutator identityMutator) {
        AccountInfoServiceProvider.init(identityManager);
        return new BraveSigninManager(identityManager);
    }

    @CalledByNative
    void destroy() {
        AccountInfoServiceProvider.get().destroy();
    }

    @Override
    public void wipeSyncUserData(Runnable wipeDataCallback, @DataWipeOption int dataWipeOption) {}

    @Override
    public void revokeSyncConsent(
            @SignoutReason int signoutSource,
            SignOutCallback signOutCallback,
            boolean forceWipeUserData) {}

    @Override
    public void signin(
            CoreAccountInfo coreAccountInfo,
            @SigninAccessPoint int accessPoint,
            @Nullable SignInCallback callback) {}

    @Deprecated
    public void turnOnSyncForTesting(
            CoreAccountInfo coreAccountInfo, @SigninAccessPoint int accessPoint) {}

    @Override
    public boolean getUserAcceptedAccountManagement() {
        return false;
    }

    @Override
    public void setUserAcceptedAccountManagement(boolean acceptedAccountManagement) {}

    @Override
    public void isAccountManaged(CoreAccountInfo account, final Callback<Boolean> callback) {}
}
