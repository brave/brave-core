/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveManageSyncSettingsClassAdapter extends BraveClassVisitor {
    static String sManageSyncSettingsClassName =
            "org/chromium/chrome/browser/sync/settings/ManageSyncSettings";
    static String sBraveManageSyncSettingsClassName =
            "org/chromium/chrome/browser/sync/settings/BraveManageSyncSettings";

    BraveManageSyncSettingsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveManageSyncSettingsClassName, "mTurnOffSync");
        makeProtectedField(sManageSyncSettingsClassName, "mTurnOffSync");

        deleteField(sBraveManageSyncSettingsClassName, "mGoogleActivityControls");
        makeProtectedField(sManageSyncSettingsClassName, "mGoogleActivityControls");

        deleteField(sBraveManageSyncSettingsClassName, "mSyncEncryption");
        makeProtectedField(sManageSyncSettingsClassName, "mSyncEncryption");

        deleteField(sBraveManageSyncSettingsClassName, "mReviewSyncData");
        makeProtectedField(sManageSyncSettingsClassName, "mReviewSyncData");

        deleteField(sBraveManageSyncSettingsClassName, "mSyncPaymentsIntegration");
        makeProtectedField(sManageSyncSettingsClassName, "mSyncPaymentsIntegration");
    }
}
