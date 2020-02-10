/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import org.chromium.chrome.browser.flags.FeatureUtilities;

/**
  * Brave's extension for BottomToolbarVariationManager
  */
public class BraveBottomToolbarVariationManager
        extends BottomToolbarVariationManager {
    private static String sBraveVariation;

    private static @Variations String getBraveVariation() {
        if (sBraveVariation != null) return sBraveVariation;
        sBraveVariation = FeatureUtilities.getBottomToolbarVariation();
        return sBraveVariation;
    }

    public static boolean isBraveVariation() {
        return FeatureUtilities.isBottomToolbarEnabled()
                && getBraveVariation().equals(Variations.NONE);
    }
}
