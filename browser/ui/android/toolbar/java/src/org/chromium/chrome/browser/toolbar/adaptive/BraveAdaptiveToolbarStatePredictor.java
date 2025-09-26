/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.adaptive;

import android.content.Context;

import org.chromium.base.BraveFeatureList;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.ui.permissions.AndroidPermissionDelegate;

/**
 * Brave-specific implementation of AdaptiveToolbarStatePredictor that extends the base
 * functionality to support Brave-specific toolbar button variants.
 */
@NullMarked
public class BraveAdaptiveToolbarStatePredictor
        extends BraveAdaptiveToolbarStatePredictorDummySuper {

    public BraveAdaptiveToolbarStatePredictor(
            Context context,
            Profile profile,
            @Nullable AndroidPermissionDelegate androidPermissionDelegate,
            @Nullable AdaptiveToolbarBehavior behavior) {
        super(context, profile, androidPermissionDelegate, behavior);
    }

    /**
     * Returns true if the given toolbar button variant is a Brave-specific valid option, otherwise
     * defers to the parent implementation.
     *
     * @param variant The button variant to validate
     * @return true if the variant is valid for Brave, otherwise call super implementation
     */
    @Override
    public boolean isValidSegment(@AdaptiveToolbarButtonVariant int variant) {
        // Check Brave-specific variants first
        switch (variant) {
                // Add more Brave-specific variants here as needed
            case AdaptiveToolbarButtonVariant.BOOKMARKS:
            case AdaptiveToolbarButtonVariant.HISTORY:
            case AdaptiveToolbarButtonVariant.DOWNLOADS:
                return true;
            case AdaptiveToolbarButtonVariant.LEO:
                return ChromeFeatureList.isEnabled(BraveFeatureList.AI_CHAT);
            case AdaptiveToolbarButtonVariant.WALLET:
                return ChromeFeatureList.isEnabled(BraveFeatureList.NATIVE_BRAVE_WALLET);
        }

        return super.isValidSegment(variant);
    }
}
