/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkManagerClassAdapter extends BraveClassVisitor {
    static String sBookmarkManagerClassName =
            "org/chromium/chrome/browser/bookmarks/BookmarkManager";
    static String sBraveBookmarkManagerClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkManager";

    public BraveBookmarkManagerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sBookmarkManagerClassName, sBraveBookmarkManagerClassName);

        deleteField(sBraveBookmarkManagerClassName, "mBookmarkModel");
        makeProtectedField(sBookmarkManagerClassName, "mBookmarkModel");
        deleteField(sBraveBookmarkManagerClassName, "mContext");
        makeProtectedField(sBookmarkManagerClassName, "mContext");
    }
}
