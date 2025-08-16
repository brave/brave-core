/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BravePasswordManagerHelperClassAdapter extends BraveClassVisitor {
    static String sChromePasswordManagerHelperClassName =
            "org/chromium/chrome/browser/password_manager/PasswordManagerHelper";
    static String sBravePasswordManagerHelperClassName =
            "org/chromium/chrome/browser/password_manager/BravePasswordManagerHelper";

    BravePasswordManagerHelperClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBravePasswordManagerHelperClassName, "sProfileMap");
        makeProtectedField(sChromePasswordManagerHelperClassName, "sProfileMap");

        changeMethodOwner(
                sChromePasswordManagerHelperClassName,
                "getForProfile",
                sBravePasswordManagerHelperClassName);
    }
}
