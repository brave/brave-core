// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.password_manager.settings;

import org.chromium.build.annotations.NullMarked;

/**
 * A class representing information about a saved password entry in Chrome's settngs.
 *
 * <p>Note: This could be a nested class in the PasswordManagerHandler interface, but that would
 * mean that PasswordUiView, which implements that interface and references SavedPasswordEntry in
 * some of its JNI-registered methods, would need an explicit import of PasswordManagerHandler. That
 * again would violate our presubmit checks, and https://crbug.com/424792 indicates that the
 * preferred solution is to move the nested class to top-level.
 */
@NullMarked
public final class SavedPasswordEntry {
    private final String mUrl;
    private final String mName;
    private final String mPassword;

    public SavedPasswordEntry(String url, String name, String password) {
        mUrl = url;
        mName = name;
        mPassword = password;
    }

    public String getUrl() {
        return mUrl;
    }

    public String getUserName() {
        return mName;
    }

    public String getPassword() {
        return mPassword;
    }
}
