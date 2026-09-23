/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAutofillClientProviderUtilsClassAdapter extends BraveClassVisitor {
    static String sAutofillClientProviderUtilsClassName =
            "org/chromium/chrome/browser/autofill/AutofillClientProviderUtils";
    static String sBraveAutofillClientProviderUtilsClassName =
            "org/chromium/chrome/browser/autofill/BraveAutofillClientProviderUtils";

    BraveAutofillClientProviderUtilsClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeMethodOwner(
                sAutofillClientProviderUtilsClassName,
                "getAndroidAutofillFrameworkAvailability",
                sBraveAutofillClientProviderUtilsClassName);
    }
}
