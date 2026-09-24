/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkToolbarClassAdapter extends BraveClassVisitor {
    static String sBookmarkToolbarClassName =
            "org/chromium/chrome/browser/bookmarks/BookmarkToolbar";
    static String sBraveBookmarkToolbarClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkToolbar";

    public BraveBookmarkToolbarClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sBookmarkToolbarClassName, sBraveBookmarkToolbarClassName);
        deleteField(sBraveBookmarkToolbarClassName, "mBookmarkModel");
        makeProtectedField(sBookmarkToolbarClassName, "mBookmarkModel");
    }
}
