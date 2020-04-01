/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAndroidSyncSettingsClassAdapter extends BraveClassVisitor {
	static String sAndroidSyncSettingsClassName =
            "org/chromium/components/sync/AndroidSyncSettings";
    static String sBraveAndroidSyncSettingsClassName =
            "org/chromium/components/sync/BraveAndroidSyncSettings";

    public BraveAndroidSyncSettingsClassAdapter(ClassVisitor visitor) {
        super(visitor);
        deleteMethod(sBraveAndroidSyncSettingsClassName,
        	"notifyObservers");
        makePublicMethod(sAndroidSyncSettingsClassName,
        	"notifyObservers");

        makePublicMethod(sAndroidSyncSettingsClassName,
            "updateCachedSettings");
        addMethodAnnotation(sBraveAndroidSyncSettingsClassName,
            "updateCachedSettings", "Ljava/lang/Override;");

        makePublicMethod(sAndroidSyncSettingsClassName,
            "setChromeSyncEnabled");
        addMethodAnnotation(sBraveAndroidSyncSettingsClassName,
            "setChromeSyncEnabled", "Ljava/lang/Override;");

        deleteField(sBraveAndroidSyncSettingsClassName, "mIsSyncable");
        makeProtectedField(sAndroidSyncSettingsClassName, "mIsSyncable");

        deleteField(sBraveAndroidSyncSettingsClassName, "mChromeSyncEnabled");
        makeProtectedField(sAndroidSyncSettingsClassName, "mChromeSyncEnabled");

        deleteField(sBraveAndroidSyncSettingsClassName, "mMasterSyncEnabled");
        makeProtectedField(sAndroidSyncSettingsClassName, "mMasterSyncEnabled");
    }
}
