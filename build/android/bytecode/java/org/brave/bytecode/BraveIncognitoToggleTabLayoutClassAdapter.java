/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveIncognitoToggleTabLayoutClassAdapter extends BraveClassVisitor {
    static String sIncognitoToggleTabLayoutClassName =
            "org/chromium/chrome/browser/toolbar/IncognitoToggleTabLayout";
    static String sBraveIncognitoToggleTabLayoutClassName =
            "org/chromium/chrome/browser/toolbar/BraveIncognitoToggleTabLayout";

    public BraveIncognitoToggleTabLayoutClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveIncognitoToggleTabLayoutClassName, "mIncognitoButtonIcon");
        makeProtectedField(sIncognitoToggleTabLayoutClassName, "mIncognitoButtonIcon");
    }
}
