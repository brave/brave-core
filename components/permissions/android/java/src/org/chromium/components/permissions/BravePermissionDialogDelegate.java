/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.permissions;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

@JNINamespace("permissions")
public class BravePermissionDialogDelegate {
    /** Lifetime options to show to the user. Can be null if no options should be shown. */
    private String[] mLifetimeOptions;

    /** Lifetime option index selected by the user. */
    private int mSelectedLifetimeOption;

    public BravePermissionDialogDelegate() {
        mSelectedLifetimeOption = -1;
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
}
