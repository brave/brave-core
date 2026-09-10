/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveIncognitoCustomTabSnapshotControllerClassAdapter extends BraveClassVisitor {
    static String sIncognitoCustomTabSnapshotControllerClassName =
            "org/chromium/chrome/browser/customtabs/IncognitoCustomTabSnapshotController";
    static String sBraveIncognitoCustomTabSnapshotControllerClassName =
            "org/chromium/chrome/browser/customtabs/BraveIncognitoCustomTabSnapshotController";

    public BraveIncognitoCustomTabSnapshotControllerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sIncognitoCustomTabSnapshotControllerClassName,
                sBraveIncognitoCustomTabSnapshotControllerClassName);
    }
}
