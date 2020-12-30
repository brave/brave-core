/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveTabbedActivityClassAdapter extends BraveClassVisitor {
    static String sChromeTabbedActivityClassName =
            "org/chromium/chrome/browser/ChromeTabbedActivity";
    static String sBraveActivityClassName = "org/chromium/chrome/browser/app/BraveActivity";
    static String sTabbedRootUiCoordinatorClassName =
            "org/chromium/chrome/browser/tabbed_mode/TabbedRootUiCoordinator";
    static String sBraveTabbedRootUiCoordinatorClassName =
            "org/chromium/chrome/browser/tabbed_mode/BraveTabbedRootUiCoordinator";
    static String sTabbedAppMenuPropertiesDelegateClassName =
            "org/chromium/chrome/browser/tabbed_mode/TabbedAppMenuPropertiesDelegate";
    static String sBraveTabbedAppMenuPropertiesDelegateClassName =
            "org/chromium/chrome/browser/appmenu/BraveTabbedAppMenuPropertiesDelegate";
    static String sChromeTabCreatorClassName =
            "org/chromium/chrome/browser/tabmodel/ChromeTabCreator";
    static String sBraveTabCreatorClassName =
            "org/chromium/chrome/browser/tabmodel/BraveTabCreator";

    public BraveTabbedActivityClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sChromeTabbedActivityClassName, sBraveActivityClassName);

        redirectConstructor(
                sTabbedRootUiCoordinatorClassName, sBraveTabbedRootUiCoordinatorClassName);

        redirectConstructor(sTabbedAppMenuPropertiesDelegateClassName,
                sBraveTabbedAppMenuPropertiesDelegateClassName);

        redirectConstructor(sChromeTabCreatorClassName, sBraveTabCreatorClassName);

        makePublicMethod(sChromeTabbedActivityClassName, "hideOverview");
    }
}
