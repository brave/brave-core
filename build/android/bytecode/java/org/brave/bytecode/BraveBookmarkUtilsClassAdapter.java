/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkUtilsClassAdapter extends BraveClassVisitor {
    static String sBookmarkUtilsClassName = "org/chromium/chrome/browser/bookmarks/BookmarkUtils";
    static String sBraveBookmarkUtilsClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkUtils";
    static String sBraveBookmarkUtilsDummySuperClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkUtilsDummySuper";

    public BraveBookmarkUtilsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        // Redirect upstream calls to addOrEditSingleBookmark to our override.
        changeMethodOwner(
                sBookmarkUtilsClassName, "addOrEditSingleBookmark", sBraveBookmarkUtilsClassName);
        // Make the private static method public so the back-call from BraveBookmarkUtils works.
        makePublicMethod(sBookmarkUtilsClassName, "addOrEditSingleBookmark");
        // Redirect the DummySuper stub back to the upstream implementation.
        changeMethodOwner(
                sBraveBookmarkUtilsDummySuperClassName,
                "addOrEditSingleBookmark",
                sBookmarkUtilsClassName);

        changeMethodOwner(sBookmarkUtilsClassName, "isSpecialFolder", sBraveBookmarkUtilsClassName);
    }
}
