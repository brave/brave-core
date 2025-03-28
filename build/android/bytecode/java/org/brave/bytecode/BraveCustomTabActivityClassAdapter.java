/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveCustomTabActivityClassAdapter extends BraveClassVisitor {
    static String sCustomTabActivity = "org/chromium/chrome/browser/customtabs/CustomTabActivity";
    static String sFullScreenCustomTabActivity =
            "org/chromium/chrome/browser/customtabs/FullScreenCustomTabActivity";

    public BraveCustomTabActivityClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sFullScreenCustomTabActivity, "mIsEnterAnimationCompleted");
        makeProtectedField(sCustomTabActivity, "mIsEnterAnimationCompleted");
    }
}
