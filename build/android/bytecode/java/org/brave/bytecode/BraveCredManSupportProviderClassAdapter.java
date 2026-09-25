/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveCredManSupportProviderClassAdapter extends BraveClassVisitor {
    static String sCredManSupportProviderClassName =
            "org/chromium/components/webauthn/cred_man/CredManSupportProvider";
    static String sBraveCredManSupportProviderClassName =
            "org/chromium/components/webauthn/cred_man/BraveCredManSupportProvider";
    static String sBraveCredManSupportProviderDummySuperClassName =
            "org/chromium/components/webauthn/cred_man/BraveCredManSupportProviderDummySuper";

    public BraveCredManSupportProviderClassAdapter(ClassVisitor visitor) {
        super(visitor);

        // Redirect upstream calls to hasOldGmsVersion to our override.
        changeMethodOwner(
                sCredManSupportProviderClassName,
                "hasOldGmsVersion",
                sBraveCredManSupportProviderClassName);
        // Make the private static method public so the back-call from
        // BraveCredManSupportProvider works.
        makePublicMethod(sCredManSupportProviderClassName, "hasOldGmsVersion");
        // Redirect the DummySuper stub back to the upstream implementation.
        changeMethodOwner(
                sBraveCredManSupportProviderDummySuperClassName,
                "hasOldGmsVersion",
                sCredManSupportProviderClassName);
    }
}
