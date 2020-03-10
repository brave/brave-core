/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkModelClassAdapter extends BraveClassVisitor {

    static String sBraveBookmarkModelClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkModel";

    static String sBookmarksBridgeClassName =
            "org/chromium/chrome/browser/bookmarks/BookmarkBridge";

    public BraveBookmarkModelClassAdapter(ClassVisitor visitor) {
        super(visitor);
        deleteMethod(sBraveBookmarkModelClassName,
                     "extensiveBookmarkChangesBeginning");
        deleteMethod(sBraveBookmarkModelClassName,
                     "extensiveBookmarkChangesEnded");
        deleteMethod(sBraveBookmarkModelClassName,
                     "createBookmarkItem");
        makePublicMethod(sBookmarksBridgeClassName,
                         "extensiveBookmarkChangesBeginning");
        makePublicMethod(sBookmarksBridgeClassName,
                         "extensiveBookmarkChangesEnded");
        makePublicMethod(sBookmarksBridgeClassName,
                         "createBookmarkItem");
    }
}
