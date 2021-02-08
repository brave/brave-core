/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkUtilsClassAdapter extends BraveClassVisitor {
    static String sBookmarkUtilsClassName = "org/chromium/chrome/browser/bookmarks/BookmarkUtils";
    static String sBraveBookmarkUtilsClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkUtils";

    public BraveBookmarkUtilsClassAdapter(ClassVisitor visitor) {
        super(visitor);
        makePublicMethod(sBookmarkUtilsClassName, "showBookmarkBottomSheet");
        changeMethodOwner(
                sBraveBookmarkUtilsClassName, "showBookmarkBottomSheet", sBookmarkUtilsClassName);
        makePublicMethod(sBookmarkUtilsClassName, "addBookmarkAndShowSnackbar");
        changeMethodOwner(sBraveBookmarkUtilsClassName, "addBookmarkAndShowSnackbar",
                sBookmarkUtilsClassName);
        changeMethodOwner(
                sBookmarkUtilsClassName, "addOrEditBookmark", sBraveBookmarkUtilsClassName);
    }
}
