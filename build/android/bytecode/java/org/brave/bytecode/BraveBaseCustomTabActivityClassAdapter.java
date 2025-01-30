/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveBaseCustomTabActivityClassAdapter extends BraveClassVisitor {
    static String sBaseCustomTabActivityClassName =
            "org/chromium/chrome/browser/customtabs/BaseCustomTabActivity";
    static String sRewardsPageActivityClassName =
            "org/chromium/chrome/browser/rewards/RewardsPageActivity";

    public BraveBaseCustomTabActivityClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sRewardsPageActivityClassName, "mBaseCustomTabRootUiCoordinator");
        makeProtectedField(sBaseCustomTabActivityClassName, "mBaseCustomTabRootUiCoordinator");

        deleteField(sRewardsPageActivityClassName, "mIntentDataProvider");
        makeProtectedField(sBaseCustomTabActivityClassName, "mIntentDataProvider");

        deleteField(sRewardsPageActivityClassName, "mToolbarCoordinator");
        makeProtectedField(sBaseCustomTabActivityClassName, "mToolbarCoordinator");

        deleteField(sRewardsPageActivityClassName, "mTabController");
        makeProtectedField(sBaseCustomTabActivityClassName, "mTabController");

        deleteField(sRewardsPageActivityClassName, "mMinimizationManagerHolder");
        makeProtectedField(sBaseCustomTabActivityClassName, "mMinimizationManagerHolder");

        deleteField(sRewardsPageActivityClassName, "mCustomTabFeatureOverridesManager");
        makeProtectedField(sBaseCustomTabActivityClassName, "mCustomTabFeatureOverridesManager");
    }
}
