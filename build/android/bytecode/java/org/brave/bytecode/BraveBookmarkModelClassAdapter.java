/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkModelClassAdapter extends BraveClassVisitor {
    static String sBraveBookmarkBridgeClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkBridge";
    static String sBookmarkModelClassName = "org/chromium/chrome/browser/bookmarks/BookmarkModel";
    static String sBraveBookmarkModelClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkModel";

    public BraveBookmarkModelClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sBookmarkModelClassName, sBraveBookmarkBridgeClassName);
        redirectConstructor(sBookmarkModelClassName, sBraveBookmarkModelClassName);
        deleteMethod(sBraveBookmarkModelClassName, "importBookmarks");
        deleteMethod(sBraveBookmarkModelClassName, "exportBookmarks");
    }
}
