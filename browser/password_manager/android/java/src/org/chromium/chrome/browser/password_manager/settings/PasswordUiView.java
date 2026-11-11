// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_manager.settings;

import static org.chromium.build.NullUtil.assumeNonNull;

import android.content.Context;

import org.jni_zero.CalledByNative;
import org.jni_zero.JniPtr;
import org.jni_zero.JniRawPtr;
import org.jni_zero.JniType;
import org.jni_zero.JniTypeToken;
import org.jni_zero.NativeMethods;

import org.chromium.base.Callback;
import org.chromium.base.IntStringCallback;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.Profile;

/**
 * Production implementation of PasswordManagerHandler, making calls to native C++ code to retrieve
 * the data.
 */
@NullMarked
public final class PasswordUiView implements PasswordManagerHandler {
    @JniType("::PasswordUiViewAndroid")
    interface NativePasswordUiViewAndroid extends JniTypeToken {}

    @CalledByNative
    private static SavedPasswordEntry createSavedPasswordEntry(
            @JniType("std::string") String url,
            @JniType("std::string") String originUrl,
            @JniType("std::u16string") String name,
            @JniType("std::u16string") String password) {
        return new SavedPasswordEntry(url, originUrl, name, password);
    }

    // Reference to the native implementation, released in destroy().
    private @Nullable JniRawPtr<NativePasswordUiViewAndroid> mNativePasswordUiViewAndroid;

    // This class has exactly one observer, set on construction and expected to last at least as
    // long as this object (a good candidate is the owner of this object).
    private final PasswordListObserver mObserver;

    /**
     * Constructor creates the native object as well. Callers should call destroy() after usage.
     *
     * @param observer The only observer.
     * @param profile The {@link Profile} associated with these passwords.
     */
    public PasswordUiView(PasswordListObserver observer, Profile profile) {
        mNativePasswordUiViewAndroid = PasswordUiViewJni.get().init(this, profile);
        mObserver = observer;
    }

    @CalledByNative
    private void passwordListAvailable(int count) {
        mObserver.passwordListAvailable(count);
    }

    @CalledByNative
    private void passwordExceptionListAvailable(int count) {
        mObserver.passwordExceptionListAvailable(count);
    }

    @Override
    public void insertPasswordEntryForTesting(String origin, String username, String password) {
        PasswordUiViewJni.get()
                .insertPasswordEntryForTesting(
                        assumeNonNull(mNativePasswordUiViewAndroid), origin, username, password);
    }

    // Calls native to refresh password and exception lists. The native code calls back into
    // passwordListAvailable and passwordExceptionListAvailable.
    @Override
    public void updatePasswordLists() {
        PasswordUiViewJni.get().updatePasswordLists(assumeNonNull(mNativePasswordUiViewAndroid));
    }

    @Override
    public SavedPasswordEntry getSavedPasswordEntry(int index) {
        return PasswordUiViewJni.get()
                .getSavedPasswordEntry(assumeNonNull(mNativePasswordUiViewAndroid), index);
    }

    @Override
    public String getSavedPasswordException(int index) {
        return PasswordUiViewJni.get()
                .getSavedPasswordException(assumeNonNull(mNativePasswordUiViewAndroid), index);
    }

    @Override
    public void removeSavedPasswordEntry(int index) {
        PasswordUiViewJni.get()
                .handleRemoveSavedPasswordEntry(assumeNonNull(mNativePasswordUiViewAndroid), index);
    }

    @Override
    public void removeSavedPasswordException(int index) {
        PasswordUiViewJni.get()
                .handleRemoveSavedPasswordException(
                        assumeNonNull(mNativePasswordUiViewAndroid), index);
    }

    @Override
    public void serializePasswords(
            String targetPath, IntStringCallback successCallback, Callback<String> errorCallback) {
        PasswordUiViewJni.get()
                .handleSerializePasswords(
                        assumeNonNull(mNativePasswordUiViewAndroid),
                        targetPath,
                        successCallback,
                        errorCallback);
    }

    @Override
    public void showPasswordEntryEditingView(
            Context context, int index, boolean isBlockedCredential) {
        if (isBlockedCredential) {
            PasswordUiViewJni.get()
                    .handleShowBlockedCredentialView(
                            assumeNonNull(mNativePasswordUiViewAndroid), context, index);
            return;
        }
        PasswordUiViewJni.get()
                .handleShowPasswordEntryEditingView(
                        assumeNonNull(mNativePasswordUiViewAndroid), context, index);
    }

    /**
     * Returns the URL for the website for managing one's passwords without the need to use Chrome
     * with the user's profile signed in.
     *
     * @return The string with the URL.
     */
    public static String getAccountDashboardURL() {
        return PasswordUiViewJni.get().getAccountDashboardURL();
    }

    /**
     * Returns the URL of the help center article about trusted vault encryption.
     *
     * @return The string with the URL.
     */
    public static String getTrustedVaultLearnMoreURL() {
        return PasswordUiViewJni.get().getTrustedVaultLearnMoreURL();
    }

    @Override
    public boolean isWaitingForPasswordStore() {
        return PasswordUiViewJni.get()
                .isWaitingForPasswordStore(assumeNonNull(mNativePasswordUiViewAndroid));
    }

    @Override
    public void importPasswordsFromCsv(
            String csvContent, Callback<Integer> successCallback, Callback<Integer> errorCallback) {
        PasswordUiViewJni.get()
                .handleImportPasswordsFromCsv(
                        assumeNonNull(mNativePasswordUiViewAndroid),
                        csvContent,
                        successCallback,
                        errorCallback);
    }

    @Override
    public int getMaxPasswordsPerCsvFile() {
        return PasswordUiViewJni.get().getMaxPasswordsPerCsvFile();
    }

    /** Destroy the native object. */
    public void destroy() {
        if (mNativePasswordUiViewAndroid != null) {
            PasswordUiViewJni.get().destroy(mNativePasswordUiViewAndroid);
            mNativePasswordUiViewAndroid.release();
            mNativePasswordUiViewAndroid = null;
        }
    }

    @NativeMethods
    interface Natives {
        JniRawPtr<NativePasswordUiViewAndroid> init(
                PasswordUiView self, @JniType("Profile*") Profile profile);

        void insertPasswordEntryForTesting(
                JniPtr<NativePasswordUiViewAndroid> self,
                @JniType("std::u16string") String origin,
                @JniType("std::u16string") String username,
                @JniType("std::u16string") String password);

        void updatePasswordLists(JniPtr<NativePasswordUiViewAndroid> self);

        SavedPasswordEntry getSavedPasswordEntry(
                JniPtr<NativePasswordUiViewAndroid> self, int index);

        @JniType("std::string")
        String getSavedPasswordException(JniPtr<NativePasswordUiViewAndroid> self, int index);

        void handleRemoveSavedPasswordEntry(JniPtr<NativePasswordUiViewAndroid> self, int index);

        void handleRemoveSavedPasswordException(
                JniPtr<NativePasswordUiViewAndroid> self, int index);

        @JniType("std::string")
        String getAccountDashboardURL();

        @JniType("std::string")
        String getTrustedVaultLearnMoreURL();

        boolean isWaitingForPasswordStore(JniPtr<NativePasswordUiViewAndroid> self);

        void destroy(JniPtr<NativePasswordUiViewAndroid> self);

        void handleSerializePasswords(
                JniPtr<NativePasswordUiViewAndroid> self,
                @JniType("std::string") String targetPath,
                IntStringCallback successCallback,
                Callback<String> errorCallback);

        void handleShowPasswordEntryEditingView(
                JniPtr<NativePasswordUiViewAndroid> self, Context context, int index);

        void handleShowBlockedCredentialView(
                JniPtr<NativePasswordUiViewAndroid> self, Context context, int index);

        void handleImportPasswordsFromCsv(
                JniPtr<NativePasswordUiViewAndroid> self,
                @JniType("std::string") String csvContent,
                Callback<Integer> successCallback,
                Callback<Integer> errorCallback);

        int getMaxPasswordsPerCsvFile();
    }
}
