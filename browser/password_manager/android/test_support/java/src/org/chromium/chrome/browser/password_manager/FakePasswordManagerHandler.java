// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_manager;

import android.content.Context;

import androidx.annotation.Nullable;

import org.chromium.base.Callback;
import org.chromium.base.IntStringCallback;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.password_manager.settings.PasswordListObserver;
import org.chromium.chrome.browser.password_manager.settings.PasswordManagerHandler;
import org.chromium.chrome.browser.password_manager.settings.SavedPasswordEntry;

import java.util.ArrayList;

/** Fake implementation for the PasswordManagerHandler. */
@NullMarked
public final class FakePasswordManagerHandler implements PasswordManagerHandler {
    // This class has exactly one observer, set on construction and expected to last at least as
    // long as this object (a good candidate is the owner of this object).
    private final PasswordListObserver mObserver;

    // The faked contents of the password store to be displayed.
    private ArrayList<SavedPasswordEntry> mSavedPasswords = new ArrayList<>();

    // The faked contents of the saves password exceptions to be displayed.
    private ArrayList<String> mSavedPasswordExeptions = new ArrayList<>();

    // The following three data members are set once {@link #serializePasswords()} is called.
    @Nullable private IntStringCallback mExportSuccessCallback;

    @Nullable private Callback<String> mExportErrorCallback;

    @Nullable private String mExportTargetPath;

    private int mSerializationInvocationCount;

    public void setSavedPasswords(ArrayList<SavedPasswordEntry> savedPasswords) {
        mSavedPasswords = savedPasswords;
    }

    public void setSavedPasswordExceptions(ArrayList<String> savedPasswordExceptions) {
        mSavedPasswordExeptions = savedPasswordExceptions;
    }

    @Nullable
    public IntStringCallback getExportSuccessCallback() {
        return mExportSuccessCallback;
    }

    @Nullable
    public Callback<String> getExportErrorCallback() {
        return mExportErrorCallback;
    }

    @Nullable
    public String getExportTargetPath() {
        return mExportTargetPath;
    }

    /**
     * Constructor.
     *
     * @param observer The only observer.
     */
    public FakePasswordManagerHandler(PasswordListObserver observer) {
        mObserver = observer;
        mSerializationInvocationCount = 0;
    }

    /**
     * A getter for the faked contents of the password store.
     *
     * @return the faked contents of the password store.
     */
    public ArrayList<SavedPasswordEntry> getSavedPasswordEntriesForTesting() {
        return mSavedPasswords;
    }

    @Override
    public void insertPasswordEntryForTesting(String origin, String username, String password) {
        mSavedPasswords.add(new SavedPasswordEntry(origin, username, password));
    }

    // Pretends that the updated lists are |mSavedPasswords| for the saved passwords and an
    // empty list for exceptions and immediately calls the observer.
    @Override
    public void updatePasswordLists() {
        mObserver.passwordListAvailable(mSavedPasswords.size());
        mObserver.passwordExceptionListAvailable(mSavedPasswordExeptions.size());
    }

    @Override
    public SavedPasswordEntry getSavedPasswordEntry(int index) {
        return mSavedPasswords.get(index);
    }

    @Override
    public String getSavedPasswordException(int index) {
        return mSavedPasswordExeptions.get(index);
    }

    @Override
    public void removeSavedPasswordEntry(int index) {
        assert false : "Define this method before starting to use it in tests.";
    }

    @Override
    public void removeSavedPasswordException(int index) {
        assert false : "Define this method before starting to use it in tests.";
    }

    @Override
    public void serializePasswords(
            String targetPath, IntStringCallback successCallback, Callback<String> errorCallback) {
        mExportSuccessCallback = successCallback;
        mExportErrorCallback = errorCallback;
        mExportTargetPath = targetPath;
        mSerializationInvocationCount += 1;
    }

    @Override
    public void showPasswordEntryEditingView(
            Context context, int index, boolean isBlockedCredential) {
        assert false : "Define this method before starting to use it in tests.";
    }

    @Override
    public boolean isWaitingForPasswordStore() {
        return false;
    }

    public int getSerializationInvocationCount() {
        return mSerializationInvocationCount;
    }

    @Override
    public void importPasswordsFromCsv(
            String csvContent,
            Callback<Integer> successCallback,
            Callback<Integer> errorCallback) {}

    @Override
    public int getMaxPasswordsPerCsvFile() {
        return 0;
    }
}
