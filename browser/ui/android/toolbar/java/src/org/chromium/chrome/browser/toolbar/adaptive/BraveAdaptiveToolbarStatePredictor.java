/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.adaptive;

import android.content.Context;

import org.chromium.build.annotations.Nullable;

import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.ui.permissions.AndroidPermissionDelegate;

/**
 * Brave-specific implementation of AdaptiveToolbarStatePredictor that extends the base
 * functionality to support Brave-specific toolbar button variants.
 */
public class BraveAdaptiveToolbarStatePredictor extends BraveAdaptiveToolbarStatePredictorDummySuper {

    /**
     * Constructs {@code BraveAdaptiveToolbarStatePredictor}
     *
     * @param context Context to determine form-factor.
     * @param profile The {@link Profile} associated with the toolbar state.
     * @param androidPermissionDelegate used for determining if voice search can be used
     * @param behavior Embedder-specific toolbar behavior. The default one is used if {@code null}
     *     is passed.
     */
    public BraveAdaptiveToolbarStatePredictor(
            Context context,
            Profile profile,
            @Nullable AndroidPermissionDelegate androidPermissionDelegate,
            @Nullable AdaptiveToolbarBehavior behavior) {
        super(context, profile, androidPermissionDelegate, behavior);
    }

    /**
     * Brave-specific validation for toolbar button variants.
     * This method provides the logic that should be used to override the private
     * isValidSegment method in the parent class via bytecode modification.
     * 
     * @param variant The button variant to validate
     * @return true if the variant is valid for Brave, false otherwise
     */
    @Override
    public boolean isValidSegment(@AdaptiveToolbarButtonVariant int variant) {
        // Check Brave-specific variants first
        switch (variant) {
            case AdaptiveToolbarButtonVariant.BOOKMARKS:
                return true;
            // Add more Brave-specific variants here as needed
        }

        return super.isValidSegment(variant);
    }
}
