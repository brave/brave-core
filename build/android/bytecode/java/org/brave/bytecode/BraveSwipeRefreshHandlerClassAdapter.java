/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

/**
 * Adapter for asm patching of upstream's SwipeRefreshHandler and override |start| implementation
 */
public class BraveSwipeRefreshHandlerClassAdapter extends BraveClassVisitor {
    static String sSwipeRefreshHandlerClassName = "org/chromium/chrome/browser/SwipeRefreshHandler";
    static String sBraveSwipeRefreshHandlerClassName =
            "org/chromium/chrome/browser/BraveSwipeRefreshHandler";

    public BraveSwipeRefreshHandlerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sSwipeRefreshHandlerClassName, sBraveSwipeRefreshHandlerClassName);

        deleteField(sBraveSwipeRefreshHandlerClassName, "mSwipeType");
        makeProtectedField(sSwipeRefreshHandlerClassName, "mSwipeType");

        deleteField(sBraveSwipeRefreshHandlerClassName, "mTab");
        makeProtectedField(sSwipeRefreshHandlerClassName, "mTab");

        changeMethodOwner(
                sSwipeRefreshHandlerClassName, "start", sBraveSwipeRefreshHandlerClassName);
    }
}
