/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabUiFeatureUtilitiesClassAdapter extends BraveClassVisitor {
    static String sTabUiFeatureUtilitiesClassName =
            "org/chromium/chrome/browser/tasks/tab_management/TabUiFeatureUtilities";
    static String sBraveTabUiFeatureUtilitiesClassName =
            "org/chromium/chrome/browser/tasks/tab_management/BraveTabUiFeatureUtilities";

    public BraveTabUiFeatureUtilitiesClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(sTabUiFeatureUtilitiesClassName, "isGridTabSwitcherEnabled",
                sBraveTabUiFeatureUtilitiesClassName);

        changeMethodOwner(sTabUiFeatureUtilitiesClassName, "isTabGroupsAndroidEnabled",
                sBraveTabUiFeatureUtilitiesClassName);
    }
}
