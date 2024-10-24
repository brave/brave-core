/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BravePasswordSettingsBaseClassAdapter extends BraveClassVisitor {
    static String sPasswordSettingsClassName =
            "org/chromium/chrome/browser/password_manager/settings/PasswordSettings";
    static String sBravePasswordSettingsBaseClassName =
            "org/chromium/chrome/browser/password_manager/settings/BravePasswordSettingsBase";

    public BravePasswordSettingsBaseClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sPasswordSettingsClassName, sBravePasswordSettingsBaseClassName);

        changeMethodOwner(
                sPasswordSettingsClassName,
                "createCheckPasswords",
                sBravePasswordSettingsBaseClassName);
        deleteMethod(sPasswordSettingsClassName, "createCheckPasswords");

        deleteField(sPasswordSettingsClassName, "mMenu");
    }
}
