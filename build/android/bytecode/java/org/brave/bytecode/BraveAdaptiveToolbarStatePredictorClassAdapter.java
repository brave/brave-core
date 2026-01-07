/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.brave.bytecode;

import org.objectweb.asm.ClassVisitor;

public class BraveAdaptiveToolbarStatePredictorClassAdapter extends BraveClassVisitor {
    static String sAdaptiveToolbarStatePredictor =
            "org/chromium/chrome/browser/toolbar/adaptive/AdaptiveToolbarStatePredictor";
    static String sBraveAdaptiveToolbarStatePredictor =
            "org/chromium/chrome/browser/toolbar/adaptive/BraveAdaptiveToolbarStatePredictor";
    static String sBraveAdaptiveToolbarStatePredictorDummySuper =
            "org/chromium/chrome/browser/toolbar/adaptive/BraveAdaptiveToolbarStatePredictorDummySuper"; // presubmit: ignore-long-line

    public BraveAdaptiveToolbarStatePredictorClassAdapter(ClassVisitor visitor) {
        super(visitor);

        changeSuperName(sBraveAdaptiveToolbarStatePredictor, sAdaptiveToolbarStatePredictor);

        makePublicMethod(sAdaptiveToolbarStatePredictor, "isValidSegment");

        // Upstream's version we redirect to our own implementation
        changeMethodOwner(
                sAdaptiveToolbarStatePredictor,
                "isValidSegment",
                sBraveAdaptiveToolbarStatePredictor);
        // Our dummy super class we redirect to back the upstream version to be able to call it from
        // our own implementation
        changeMethodOwner(
                sBraveAdaptiveToolbarStatePredictorDummySuper,
                "isValidSegment",
                sAdaptiveToolbarStatePredictor);

        redirectConstructor(sAdaptiveToolbarStatePredictor, sBraveAdaptiveToolbarStatePredictor);
    }
}
