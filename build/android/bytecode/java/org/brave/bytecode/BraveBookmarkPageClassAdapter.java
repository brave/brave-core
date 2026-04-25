/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkPageClassAdapter extends BraveClassVisitor {
    static String sBookmarkPageClassName = "org/chromium/chrome/browser/bookmarks/BookmarkPage";
    static String sBraveBookmarkPageClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkPage";

    public BraveBookmarkPageClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sBookmarkPageClassName, sBraveBookmarkPageClassName);
        deleteField(sBraveBookmarkPageClassName, "mBookmarkManagerCoordinator");
        makeProtectedField(sBookmarkPageClassName, "mBookmarkManagerCoordinator");
    }
}
