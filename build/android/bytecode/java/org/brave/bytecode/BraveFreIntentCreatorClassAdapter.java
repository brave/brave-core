/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveFreIntentCreatorClassAdapter extends BraveClassVisitor {
    static String sFreIntentCreatorClassName =
            "org/chromium/chrome/browser/firstrun/FreIntentCreator";

    static String sBraveFreIntentCreatorClassName =
            "org/chromium/chrome/browser/firstrun/BraveFreIntentCreator";

    public BraveFreIntentCreatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sFreIntentCreatorClassName, sBraveFreIntentCreatorClassName);
    }
}
