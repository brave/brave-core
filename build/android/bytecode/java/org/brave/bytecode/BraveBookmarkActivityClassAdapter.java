/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkActivityClassAdapter extends BraveClassVisitor {
    static String sBookmarkActivityClassName =
            "org/chromium/chrome/browser/app/bookmarks/BookmarkActivity";
    static String sBraveBookmarkActivityClassName =
            "org/chromium/chrome/browser/app/bookmarks/BraveBookmarkActivity";

    public BraveBookmarkActivityClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveBookmarkActivityClassName, "mBookmarkManagerCoordinator");
        makeProtectedField(sBookmarkActivityClassName, "mBookmarkManagerCoordinator");
    }
}
