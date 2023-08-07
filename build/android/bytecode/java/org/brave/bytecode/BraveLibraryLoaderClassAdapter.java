/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

/**
 * Class for patch work on overriding upstream's
 * LibraryLoader.loadWithSystemLinkerAlreadyLocked.
 * See also BraveLibraryLoader
 */
public class BraveLibraryLoaderClassAdapter extends BraveClassVisitor {
    static String sLibraryLoaderClassName = "org/chromium/base/library_loader/LibraryLoader";
    static String sBraveLibraryLoaderClassName =
            "org/chromium/base/library_loader/BraveLibraryLoader";

    public BraveLibraryLoaderClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeMethodOwner(sLibraryLoaderClassName, "getInstance", sBraveLibraryLoaderClassName);

        deleteField(sBraveLibraryLoaderClassName, "sInstance");
        makeProtectedField(sLibraryLoaderClassName, "sInstance");

        makePublicMethod(sLibraryLoaderClassName, "loadWithSystemLinkerAlreadyLocked");

        deleteMethod(sBraveLibraryLoaderClassName, "preloadAlreadyLocked");
        makePublicMethod(sLibraryLoaderClassName, "preloadAlreadyLocked");
    }
}
