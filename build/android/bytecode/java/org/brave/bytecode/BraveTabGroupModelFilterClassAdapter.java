/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabGroupModelFilterClassAdapter extends BraveClassVisitor {
    static String sTabGroupModelFilterClassName =
            "org/chromium/chrome/browser/tabmodel/TabGroupModelFilterImpl";
    static String sBraveTabGroupModelFilterClassName =
            "org/chromium/chrome/browser/tabmodel/BraveTabGroupModelFilter";

    public BraveTabGroupModelFilterClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sTabGroupModelFilterClassName, sBraveTabGroupModelFilterClassName);

        deleteField(sTabGroupModelFilterClassName, "mIsResetting");

        changeMethodOwner(
                sTabGroupModelFilterClassName,
                "shouldUseParentIds",
                sBraveTabGroupModelFilterClassName);
    }
}
