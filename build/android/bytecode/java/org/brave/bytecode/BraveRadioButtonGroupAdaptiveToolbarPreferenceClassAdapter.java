/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveRadioButtonGroupAdaptiveToolbarPreferenceClassAdapter extends BraveClassVisitor {
    static String sRadioButtonGroupAdaptiveToolbarPreference =
            "org/chromium/chrome/browser/toolbar/adaptive/settings/RadioButtonGroupAdaptiveToolbarPreference"; // presubmit: ignore-long-line
    static String sBraveRadioButtonGroupAdaptiveToolbarPreference =
            "org/chromium/chrome/browser/toolbar/adaptive/settings/BraveRadioButtonGroupAdaptiveToolbarPreference"; // presubmit: ignore-long-line

    public BraveRadioButtonGroupAdaptiveToolbarPreferenceClassAdapter(ClassVisitor visitor) {
        super(visitor);

        deleteField(sBraveRadioButtonGroupAdaptiveToolbarPreference, "mSelected");
        makeProtectedField(sRadioButtonGroupAdaptiveToolbarPreference, "mSelected");

        deleteField(sBraveRadioButtonGroupAdaptiveToolbarPreference, "mIsBound");
        makeProtectedField(sRadioButtonGroupAdaptiveToolbarPreference, "mIsBound");

        deleteField(sBraveRadioButtonGroupAdaptiveToolbarPreference, "mAutoButton");
        makeProtectedField(sRadioButtonGroupAdaptiveToolbarPreference, "mAutoButton");

        deleteField(sBraveRadioButtonGroupAdaptiveToolbarPreference, "mNewTabButton");
        makeProtectedField(sRadioButtonGroupAdaptiveToolbarPreference, "mNewTabButton");

        deleteField(sBraveRadioButtonGroupAdaptiveToolbarPreference, "mShareButton");
        makeProtectedField(sRadioButtonGroupAdaptiveToolbarPreference, "mShareButton");

        makePublicMethod(sRadioButtonGroupAdaptiveToolbarPreference, "buildUiStateForStats");
        addMethodAnnotation(
                sBraveRadioButtonGroupAdaptiveToolbarPreference,
                "buildUiStateForStats",
                "Ljava/lang/Override;");
    }
}
