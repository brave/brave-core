/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAutofillPopupBridgeClassAdapter extends BraveClassVisitor {
    static String sAutofillPopupBridgeClassName =
            "org/chromium/chrome/browser/autofill/AutofillPopupBridge";
    static String sBraveAutofillPopupBridgeClassName =
            "org/chromium/chrome/browser/autofill/BraveAutofillPopupBridge";

    public BraveAutofillPopupBridgeClassAdapter(ClassVisitor visitor) {
        super(visitor);

        redirectConstructor(sAutofillPopupBridgeClassName, sBraveAutofillPopupBridgeClassName);
    }
}
