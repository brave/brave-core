/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.permissions;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;

/* PermissionDialogDelegate override */
@JNINamespace("permissions")
public class BravePermissionDialogDelegate {
    /** Text to show before lifetime options. */
    private String mLifetimeOptionsText;

    /** Lifetime options to show to the user. Can be null if no options should be shown. */
    private String[] mLifetimeOptions;

    /** Lifetime option index selected by the user. */
    private int mSelectedLifetimeOption;

    /** Whether the request is for Widevine. We need a special layout for it. */
    private boolean mIsWidevinePermissionRequest;

    /** Don't ask again checkbox fors reqeusts. */
    private boolean mDontAskAgain;

    public BravePermissionDialogDelegate() {
        mSelectedLifetimeOption = -1;
        mIsWidevinePermissionRequest = false;
        mDontAskAgain = false;
    }

    @CalledByNative
    public void setLifetimeOptionsText(String lifetimeOptionsText) {
        mLifetimeOptionsText = lifetimeOptionsText;
    }

    public String getLifetimeOptionsText() {
        return mLifetimeOptionsText;
    }

    @CalledByNative
    public void setLifetimeOptions(String[] lifetimeOptions) {
        mLifetimeOptions = lifetimeOptions;
    }

    public String[] getLifetimeOptions() {
        return mLifetimeOptions;
    }

    public void setSelectedLifetimeOption(int idx) {
        mSelectedLifetimeOption = idx;
    }

    @CalledByNative
    public int getSelectedLifetimeOption() {
        return mSelectedLifetimeOption;
    }

    @CalledByNative
    public void setIsWidevinePermissionRequest(boolean isWidevineRequest) {
        mIsWidevinePermissionRequest = isWidevineRequest;
    }

    public boolean getIsWidevinePermissionRequest() {
        return mIsWidevinePermissionRequest;
    }

    public void setDontAskAgain(boolean checked) {
        mDontAskAgain = checked;
    }

    @CalledByNative
    public boolean getDontAskAgain() {
        return mDontAskAgain;
    }
}
