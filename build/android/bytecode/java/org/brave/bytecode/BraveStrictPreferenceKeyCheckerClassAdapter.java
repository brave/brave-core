/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveStrictPreferenceKeyCheckerClassAdapter extends BraveClassVisitor {
    static String sStrictPreferenceKeyCheckerClassName =
            "org/chromium/chrome/browser/preferences/StrictPreferenceKeyChecker";

    static String sBraveStrictPreferenceKeyCheckerClassName =
            "org/chromium/chrome/browser/preferences/BraveStrictPreferenceKeyChecker";

    public BraveStrictPreferenceKeyCheckerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sStrictPreferenceKeyCheckerClassName, sBraveStrictPreferenceKeyCheckerClassName);

        deleteMethod(sBraveStrictPreferenceKeyCheckerClassName, "isKeyInUse");
        makePublicMethod(sStrictPreferenceKeyCheckerClassName, "isKeyInUse");
    }
}
