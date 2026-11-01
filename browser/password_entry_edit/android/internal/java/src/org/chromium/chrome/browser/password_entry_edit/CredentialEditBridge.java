// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_entry_edit;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.content.Context;

import androidx.fragment.app.Fragment;

import org.jni_zero.CalledByNative;
import org.jni_zero.JniPtr;
import org.jni_zero.JniRawPtr;
import org.jni_zero.JniType;
import org.jni_zero.JniTypeToken;
import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.MonotonicNonNull;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.password_entry_edit.CredentialEditCoordinator.CredentialActionDelegate;
import org.chromium.chrome.browser.password_entry_edit.CredentialEditCoordinator.UiDismissalHandler;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.components.browser_ui.settings.SettingsNavigation;

/**
 * Class mediating the communication between the credential edit UI and the C++ part responsible for
 * saving the changes.
 */
@NullMarked
class CredentialEditBridge implements UiDismissalHandler, CredentialActionDelegate {
    @JniType("::CredentialEditBridge")
    interface NativeCredentialEditBridge extends JniTypeToken {}

    private static @Nullable CredentialEditBridge sCredentialEditBridge;

    private @Nullable JniRawPtr<NativeCredentialEditBridge> mNativeCredentialEditBridge;
    private @MonotonicNonNull CredentialEditCoordinator mCoordinator;

    static @Nullable CredentialEditBridge get() {
        return sCredentialEditBridge;
    }

    private CredentialEditBridge() {}

    @CalledByNative
    static @Nullable CredentialEditBridge maybeCreate() {
        // There can only be one bridge at a time and it shouldn't be shared.
        if (sCredentialEditBridge != null) return null;
        sCredentialEditBridge = new CredentialEditBridge();
        return sCredentialEditBridge;
    }

    @CalledByNative
    void initAndLaunchUi(
            JniRawPtr<NativeCredentialEditBridge> nativeCredentialEditBridge,
            Context context,
            boolean isBlockedCredential,
            boolean isFederatedCredential) {
        mNativeCredentialEditBridge = nativeCredentialEditBridge;
        SettingsNavigation settingsNavigation =
                SettingsNavigationFactory.createSettingsNavigation();
        // Add the credential page to the back stack so that, when settings runs as a single
        // activity (e.g. multi-column layout on tablets), it is shown within the current Settings
        // activity - nested under the password list - instead of launching a separate activity.
        Class<? extends Fragment> fragment;
        if (isBlockedCredential) {
            fragment = BlockedCredentialFragmentView.class;
        } else if (isFederatedCredential) {
            fragment = FederatedCredentialFragmentView.class;
        } else {
            fragment = CredentialEditFragmentView.class;
        }
        settingsNavigation.startSettings(
                context, fragment, /* fragmentArgs= */ null, /* addToBackStack= */ true);
    }

    public void initialize(CredentialEditCoordinator coordinator) {
        mCoordinator = coordinator;
        // This will result in setCredential being called from native with the required data.
        CredentialEditBridgeJni.get().getCredential(assumeNonNull(mNativeCredentialEditBridge));

        // This will result in setExistingUsernames being called from native with the required data.
        CredentialEditBridgeJni.get()
                .getExistingUsernames(assumeNonNull(mNativeCredentialEditBridge));
    }

    @CalledByNative
    void setCredential(
            @JniType("std::u16string") String displayUrlOrAppName,
            @JniType("std::u16string") String username,
            @JniType("std::u16string") String password,
            @JniType("std::u16string") String displayFederationOrigin,
            boolean isInsecureCredential) {
        assumeNonNull(mCoordinator);
        mCoordinator.setCredential(
                displayUrlOrAppName,
                username,
                password,
                displayFederationOrigin,
                isInsecureCredential);
    }

    @CalledByNative
    void setExistingUsernames(String[] existingUsernames) {
        assumeNonNull(mCoordinator);
        mCoordinator.setExistingUsernames(existingUsernames);
    }

    // This can be called either before or after the native counterpart has gone away, depending
    // on where the edit component is being destroyed from.
    @Override
    public void onUiDismissed() {
        if (mNativeCredentialEditBridge != null) {
            CredentialEditBridgeJni.get().onUiDismissed(mNativeCredentialEditBridge);
            releaseNativePtr();
        }
        sCredentialEditBridge = null;
    }

    @Override
    public void saveChanges(String username, String password) {
        if (mNativeCredentialEditBridge == null) return;
        CredentialEditBridgeJni.get().saveChanges(mNativeCredentialEditBridge, username, password);
    }

    @Override
    public void deleteCredential() {
        CredentialEditBridgeJni.get().deleteCredential(assumeNonNull(mNativeCredentialEditBridge));
    }

    @CalledByNative
    void destroy() {
        if (mCoordinator != null) mCoordinator.dismiss();
        releaseNativePtr();
        sCredentialEditBridge = null;
    }

    private void releaseNativePtr() {
        if (mNativeCredentialEditBridge != null) {
            mNativeCredentialEditBridge.release();
            mNativeCredentialEditBridge = null;
        }
    }

    @NativeMethods
    interface Natives {
        void getCredential(JniPtr<NativeCredentialEditBridge> self);

        void getExistingUsernames(JniPtr<NativeCredentialEditBridge> self);

        void saveChanges(
                JniPtr<NativeCredentialEditBridge> self,
                @JniType("std::u16string") String username,
                @JniType("std::u16string") String password);

        void deleteCredential(JniPtr<NativeCredentialEditBridge> self);

        void onUiDismissed(JniPtr<NativeCredentialEditBridge> self);
    }
}
