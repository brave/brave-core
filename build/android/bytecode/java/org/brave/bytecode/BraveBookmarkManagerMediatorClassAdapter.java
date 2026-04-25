/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkManagerMediatorClassAdapter extends BraveClassVisitor {
    static String sBookmarkManagerMediatorClassName =
            "org/chromium/chrome/browser/bookmarks/BookmarkManagerMediator";
    static String sBraveBookmarkManagerMediatorClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkManagerMediator";

    public BraveBookmarkManagerMediatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(
                sBookmarkManagerMediatorClassName, sBraveBookmarkManagerMediatorClassName);

        deleteField(sBraveBookmarkManagerMediatorClassName, "mBookmarkModel");
        makeProtectedField(sBookmarkManagerMediatorClassName, "mBookmarkModel");
        deleteField(sBraveBookmarkManagerMediatorClassName, "mContext");
        makeProtectedField(sBookmarkManagerMediatorClassName, "mContext");
    }
}
