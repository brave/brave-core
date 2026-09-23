/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabCollectionTabModelImplClassAdapter extends BraveClassVisitor {
    static String sTabCollectionTabModelImplClassName =
            "org/chromium/chrome/browser/tabmodel/TabCollectionTabModelImpl";
    static String sBraveTabCollectionTabModelImplBaseClassName =
            "org/chromium/chrome/browser/tabmodel/BraveTabCollectionTabModelImplBase";

    public BraveTabCollectionTabModelImplClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(
                sTabCollectionTabModelImplClassName, sBraveTabCollectionTabModelImplBaseClassName);

        changeMethodOwner(
                sTabCollectionTabModelImplClassName,
                "shouldGroupWithParent",
                sBraveTabCollectionTabModelImplBaseClassName);
    }
}
