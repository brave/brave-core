/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveToolbarManagerClassAdapter extends BraveClassVisitor {
    static String sToolbarManagerClassName = "org/chromium/chrome/browser/toolbar/ToolbarManager";
    static String sBraveToolbarManagerClassName =
            "org/chromium/chrome/browser/toolbar/BraveToolbarManager";

    public BraveToolbarManagerClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sToolbarManagerClassName, sBraveToolbarManagerClassName);

        deleteField(sBraveToolbarManagerClassName, "mBottomControlsCoordinatorSupplier");
        makeProtectedField(sToolbarManagerClassName, "mBottomControlsCoordinatorSupplier");

        deleteField(sBraveToolbarManagerClassName, "mCallbackController");
        makeProtectedField(sToolbarManagerClassName, "mCallbackController");

        deleteField(sBraveToolbarManagerClassName, "mBrowserControlsSizer");
        makeProtectedField(sToolbarManagerClassName, "mBrowserControlsSizer");

        deleteField(sBraveToolbarManagerClassName, "mFullscreenManager");
        makeProtectedField(sToolbarManagerClassName, "mFullscreenManager");

        deleteField(sBraveToolbarManagerClassName, "mActivityTabProvider");
        makeProtectedField(sToolbarManagerClassName, "mActivityTabProvider");

        deleteField(sBraveToolbarManagerClassName, "mAppThemeColorProvider");
        makeProtectedField(sToolbarManagerClassName, "mAppThemeColorProvider");

        deleteField(sBraveToolbarManagerClassName, "mShareDelegateSupplier");
        makeProtectedField(sToolbarManagerClassName, "mShareDelegateSupplier");

        deleteField(sBraveToolbarManagerClassName, "mScrimCoordinator");
        makeProtectedField(sToolbarManagerClassName, "mScrimCoordinator");

        deleteField(sBraveToolbarManagerClassName, "mShowStartSurfaceSupplier");
        makeProtectedField(sToolbarManagerClassName, "mShowStartSurfaceSupplier");

        deleteField(sBraveToolbarManagerClassName, "mMenuButtonCoordinator");
        makeProtectedField(sToolbarManagerClassName, "mMenuButtonCoordinator");

        deleteField(sBraveToolbarManagerClassName, "mToolbarTabController");
        makeProtectedField(sToolbarManagerClassName, "mToolbarTabController");

        deleteField(sBraveToolbarManagerClassName, "mLocationBar");
        makeProtectedField(sToolbarManagerClassName, "mLocationBar");

        deleteField(sBraveToolbarManagerClassName, "mActionModeController");
        makeProtectedField(sToolbarManagerClassName, "mActionModeController");

        deleteField(sBraveToolbarManagerClassName, "mLocationBarModel");
        makeProtectedField(sToolbarManagerClassName, "mLocationBarModel");

        deleteField(sBraveToolbarManagerClassName, "mToolbar");
        makeProtectedField(sToolbarManagerClassName, "mToolbar");

        deleteField(sBraveToolbarManagerClassName, "mBookmarkBridgeSupplier");
        makeProtectedField(sToolbarManagerClassName, "mBookmarkBridgeSupplier");

        deleteField(sBraveToolbarManagerClassName, "mLayoutManager");
        makeProtectedField(sToolbarManagerClassName, "mLayoutManager");

        deleteField(sBraveToolbarManagerClassName, "mOverlayPanelVisibilitySupplier");
        makeProtectedField(sToolbarManagerClassName, "mOverlayPanelVisibilitySupplier");

        deleteField(sBraveToolbarManagerClassName, "mTabModelSelector");
        makeProtectedField(sToolbarManagerClassName, "mTabModelSelector");

        deleteField(sBraveToolbarManagerClassName, "mIncognitoStateProvider");
        makeProtectedField(sToolbarManagerClassName, "mIncognitoStateProvider");

        deleteField(sBraveToolbarManagerClassName, "mTabCountProvider");
        makeProtectedField(sToolbarManagerClassName, "mTabCountProvider");

        deleteField(sBraveToolbarManagerClassName, "mTabGroupUi");
        makeProtectedField(sToolbarManagerClassName, "mTabGroupUi");

        deleteField(sBraveToolbarManagerClassName, "mBottomSheetController");
        makeProtectedField(sToolbarManagerClassName, "mBottomSheetController");

        deleteField(sBraveToolbarManagerClassName, "mActivityLifecycleDispatcher");
        makeProtectedField(sToolbarManagerClassName, "mActivityLifecycleDispatcher");

        deleteField(sBraveToolbarManagerClassName, "mIsWarmOnResumeSupplier");
        makeProtectedField(sToolbarManagerClassName, "mIsWarmOnResumeSupplier");

        deleteField(sBraveToolbarManagerClassName, "mTabContentManager");
        makeProtectedField(sToolbarManagerClassName, "mTabContentManager");

        deleteField(sBraveToolbarManagerClassName, "mTabCreatorManager");
        makeProtectedField(sToolbarManagerClassName, "mTabCreatorManager");

        deleteField(sBraveToolbarManagerClassName, "mOverviewModeBehaviorSupplier");
        makeProtectedField(sToolbarManagerClassName, "mOverviewModeBehaviorSupplier");

        deleteField(sBraveToolbarManagerClassName, "mSnackbarManager");
        makeProtectedField(sToolbarManagerClassName, "mSnackbarManager");

        makePublicMethod(sToolbarManagerClassName, "onOrientationChange");
        addMethodAnnotation(
                sBraveToolbarManagerClassName, "onOrientationChange", "Ljava/lang/Override;");

        makePublicMethod(sToolbarManagerClassName, "updateButtonStatus");
        addMethodAnnotation(
                sBraveToolbarManagerClassName, "updateButtonStatus", "Ljava/lang/Override;");

        makePublicMethod(sToolbarManagerClassName, "updateBookmarkButtonStatus");
        addMethodAnnotation(sBraveToolbarManagerClassName, "updateBookmarkButtonStatus",
                "Ljava/lang/Override;");

        makePublicMethod(sToolbarManagerClassName, "updateReloadState");
        deleteMethod(sBraveToolbarManagerClassName, "updateReloadState");
    }
}
