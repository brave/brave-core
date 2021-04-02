/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveFeedSurfaceCoordinatorClassAdapter extends BraveClassVisitor {
    static String sFeedSurfaceCoordinatorClassName =
            "org/chromium/chrome/browser/feed/FeedSurfaceCoordinator";
    static String sBraveFeedSurfaceCoordinatorClassName =
            "org/chromium/chrome/browser/feed/BraveFeedSurfaceCoordinator";

    public BraveFeedSurfaceCoordinatorClassAdapter(ClassVisitor visitor) {
        super(visitor);
        makePublicMethod(sFeedSurfaceCoordinatorClassName, "isEnhancedProtectionPromoEnabled");
        addMethodAnnotation(sBraveFeedSurfaceCoordinatorClassName,
                "isEnhancedProtectionPromoEnabled", "Ljava/lang/Override;");

        deleteField(sBraveFeedSurfaceCoordinatorClassName, "mActivity");
        makeProtectedField(sFeedSurfaceCoordinatorClassName, "mActivity");

        deleteField(sBraveFeedSurfaceCoordinatorClassName, "mScrollViewForPolicy");
        makeProtectedField(sFeedSurfaceCoordinatorClassName, "mScrollViewForPolicy");

        deleteField(sBraveFeedSurfaceCoordinatorClassName, "mNtpHeader");
        makeProtectedField(sFeedSurfaceCoordinatorClassName, "mNtpHeader");

        deleteField(sBraveFeedSurfaceCoordinatorClassName, "mRootView");
        makeProtectedField(sFeedSurfaceCoordinatorClassName, "mRootView");
    }
}
