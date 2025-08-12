/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BravePasswordsPreferenceClassAdapter extends BraveClassVisitor {
    static String sPasswordsPreferenceClassName =
            "org/chromium/chrome/browser/password_manager/settings/PasswordsPreference";

    static String sBravePasswordsPreferenceClassName =
            "org/chromium/chrome/browser/password_manager/settings/BravePasswordsPreference";

    public BravePasswordsPreferenceClassAdapter(ClassVisitor visitor) {
        super(visitor);

        makePublicMethod(sPasswordsPreferenceClassName, "setUpPostDeprecationWarning");
        addMethodAnnotation(
                sBravePasswordsPreferenceClassName,
                "setUpPostDeprecationWarning",
                "Ljava/lang/Override;");
    }
}
