/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkToolbarCoordinatorClassAdapter extends BraveClassVisitor {
    static String sBookmarkToolbarCoordinatorClassName =
            "org/chromium/chrome/browser/bookmarks/BookmarkToolbarCoordinator";
    static String sBraveBookmarkToolbarCoordinatorClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkToolbarCoordinator";

    public BraveBookmarkToolbarCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sBookmarkToolbarCoordinatorClassName, sBraveBookmarkToolbarCoordinatorClassName);

        deleteField(sBraveBookmarkToolbarCoordinatorClassName, "mToolbar");
        makeProtectedField(sBookmarkToolbarCoordinatorClassName, "mToolbar");
    }
}
