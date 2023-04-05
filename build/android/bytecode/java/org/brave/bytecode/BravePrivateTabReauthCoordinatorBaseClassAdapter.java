/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BravePrivateTabReauthCoordinatorBaseClassAdapter extends BraveClassVisitor {
    static String sTabSwitcherIncognitoReauthCoordinatorClassName =
            "org/chromium/chrome/browser/incognito/reauth/TabSwitcherIncognitoReauthCoordinator";

    static String sFullScreenIncognitoReauthCoordinatorClassName =
            "org/chromium/chrome/browser/incognito/reauth/FullScreenIncognitoReauthCoordinator";

    static String sIncognitoReauthCoordinatorBaseClassName =
            "org/chromium/chrome/browser/incognito/reauth/IncognitoReauthCoordinatorBase";

    static String sBravePrivateTabReauthCoordinatorBaseClassName =
            "org/chromium/chrome/browser/incognito/reauth/BravePrivateTabReauthCoordinatorBase";

    public BravePrivateTabReauthCoordinatorBaseClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sTabSwitcherIncognitoReauthCoordinatorClassName,
                sBravePrivateTabReauthCoordinatorBaseClassName);

        changeSuperName(sFullScreenIncognitoReauthCoordinatorClassName,
                sBravePrivateTabReauthCoordinatorBaseClassName);

        deleteField(sBravePrivateTabReauthCoordinatorBaseClassName, "mIncognitoReauthView");
        makeProtectedField(sIncognitoReauthCoordinatorBaseClassName, "mIncognitoReauthView");
    }
}
