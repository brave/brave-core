/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkBridgeClassAdapter extends BraveClassVisitor {
    static String sBookmarkBridgeClassName = "org/chromium/chrome/browser/bookmarks/BookmarkBridge";
    static String sBraveBookmarkBridgeClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkBridge";

    public BraveBookmarkBridgeClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sBookmarkBridgeClassName, sBraveBookmarkBridgeClassName);
        deleteField(sBraveBookmarkBridgeClassName, "mNativeBookmarkBridge");
        makeProtectedField(sBookmarkBridgeClassName, "mNativeBookmarkBridge");
    }
}
