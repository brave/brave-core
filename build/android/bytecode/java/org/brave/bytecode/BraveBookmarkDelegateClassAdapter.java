/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBookmarkDelegateClassAdapter extends BraveClassVisitor {
    static String sBookmarkDelegateClassName =
            "org/chromium/chrome/browser/bookmarks/BookmarkDelegate";
    static String sBraveBookmarkDelegateClassName =
            "org/chromium/chrome/browser/bookmarks/BraveBookmarkDelegate";

    public BraveBookmarkDelegateClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sBookmarkDelegateClassName, sBraveBookmarkDelegateClassName);
    }
}
