/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveLocationBarMediatorClassAdapter extends BraveClassVisitor {
    static String sLocationBarMediator = "org/chromium/chrome/browser/omnibox/LocationBarMediator";
    static String sBraveLocationBarMediator =
            "org/chromium/chrome/browser/omnibox/BraveLocationBarMediator";

    public BraveLocationBarMediatorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sLocationBarMediator, sBraveLocationBarMediator);

        deleteMethod(sBraveLocationBarMediator, "shouldShowDeleteButton");
        makePublicMethod(sLocationBarMediator, "shouldShowDeleteButton");

        deleteField(sBraveLocationBarMediator, "mNativeInitialized");
        makeProtectedField(sLocationBarMediator, "mNativeInitialized");
        deleteField(sBraveLocationBarMediator, "mWindowAndroid");
        makeProtectedField(sLocationBarMediator, "mWindowAndroid");
        deleteField(sBraveLocationBarMediator, "mLocationBarLayout");
        makeProtectedField(sLocationBarMediator, "mLocationBarLayout");
        deleteField(sBraveLocationBarMediator, "mIsUrlFocusChangeInProgress");
        makeProtectedField(sLocationBarMediator, "mIsUrlFocusChangeInProgress");
        deleteField(sBraveLocationBarMediator, "mUrlHasFocus");
        makeProtectedField(sLocationBarMediator, "mUrlHasFocus");
        deleteField(sBraveLocationBarMediator, "mIsTablet");
        makeProtectedField(sLocationBarMediator, "mIsTablet");
        deleteField(sBraveLocationBarMediator, "mIsLocationBarFocusedFromNtpScroll");
        makeProtectedField(sLocationBarMediator, "mIsLocationBarFocusedFromNtpScroll");
        deleteField(sBraveLocationBarMediator, "mContext");
        makeProtectedField(sLocationBarMediator, "mContext");
        deleteField(sBraveLocationBarMediator, "mBrandedColorScheme");
        makeProtectedField(sLocationBarMediator, "mBrandedColorScheme");
        deleteField(sBraveLocationBarMediator, "mAssistantVoiceSearchServiceSupplier");
        makeProtectedField(sLocationBarMediator, "mAssistantVoiceSearchServiceSupplier");
    }
}
