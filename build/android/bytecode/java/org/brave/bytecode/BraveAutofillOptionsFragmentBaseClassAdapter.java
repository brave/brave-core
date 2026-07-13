/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAutofillOptionsFragmentBaseClassAdapter extends BraveClassVisitor {
    static String sAutofillOptionsFragmentClassName =
            "org/chromium/chrome/browser/autofill/settings/options/AutofillOptionsFragment";

    static String sBraveAutofillOptionsFragmentBaseClassName =
            "org/chromium/chrome/browser/autofill/settings/options/BraveAutofillOptionsFragmentBase"; // presubmit: ignore-long-line

    public BraveAutofillOptionsFragmentBaseClassAdapter(ClassVisitor visitor) {
        super(visitor);
        changeSuperName(
                sAutofillOptionsFragmentClassName, sBraveAutofillOptionsFragmentBaseClassName);
    }
}
