/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveRadioButtonGroupHomepagePreferenceClassAdapter extends BraveClassVisitor {
    static String sRadioButtonGroupHomepagePreferenceClassName =
            "org/chromium/chrome/browser/homepage/settings/RadioButtonGroupHomepagePreference";
    static String sBraveRadioButtonGroupHomepagePreferenceClassName =
            "org/chromium/chrome/browser/homepage/settings/BraveRadioButtonGroupHomepagePreference";

    public BraveRadioButtonGroupHomepagePreferenceClassAdapter(ClassVisitor visitor) {
        super(visitor);

        makeNonFinalClass(sRadioButtonGroupHomepagePreferenceClassName);

        changeSuperName(
                sBraveRadioButtonGroupHomepagePreferenceClassName,
                sRadioButtonGroupHomepagePreferenceClassName);
    }
}
