/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveGmsCoreUtilsClassAdapter extends BraveClassVisitor {
    static String sGmsCoreUtilsClassName = "org/chromium/components/webauthn/GmsCoreUtils";
    static String sBraveGmsCoreUtilsClassName =
            "org/chromium/components/webauthn/BraveGmsCoreUtils";

    public BraveGmsCoreUtilsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        // Redirect upstream calls to isWebauthnSupported to our override, which also accepts the
        // platform Credential Manager as a WebAuthn backend when Play Services is unavailable.
        changeMethodOwner(
                sGmsCoreUtilsClassName, "isWebauthnSupported", sBraveGmsCoreUtilsClassName);
    }
}
