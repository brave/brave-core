/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveFeedSurfaceMediatorClassAdapter extends BraveClassVisitor {
    static String sFeedSurfaceMediatorClassName =
            "org/chromium/chrome/browser/feed/FeedSurfaceMediator";
    static String sBraveFeedSurfaceMediatorClassName =
            "org/chromium/chrome/browser/feed/BraveFeedSurfaceMediator";

    public BraveFeedSurfaceMediatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sFeedSurfaceMediatorClassName, sBraveFeedSurfaceMediatorClassName);

        deleteField(sBraveFeedSurfaceMediatorClassName, "mCoordinator");
        makeProtectedField(sFeedSurfaceMediatorClassName, "mCoordinator");

        deleteField(sBraveFeedSurfaceMediatorClassName, "mSnapScrollHelper");
        makeProtectedField(sFeedSurfaceMediatorClassName, "mSnapScrollHelper");
    }
}
