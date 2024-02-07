/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BravePartialCustomTabBottomSheetStrategyClassAdapter extends BraveClassVisitor {
    static String sNamespace = "org/chromium/chrome/browser/customtabs/features/partialcustomtab/";
    static String sStrategy = sNamespace + "PartialCustomTabBottomSheetStrategy";
    static String sBraveStrategy = sNamespace + "BravePartialCustomTabBottomSheetStrategy";

    BravePartialCustomTabBottomSheetStrategyClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sStrategy, sBraveStrategy);

        deleteField(sBraveStrategy, "mStopShowingSpinner");
        makeProtectedField(sStrategy, "mStopShowingSpinner");

        deleteField(sBraveStrategy, "mTab");
        makeProtectedField(sStrategy, "mTab");
    }
}
