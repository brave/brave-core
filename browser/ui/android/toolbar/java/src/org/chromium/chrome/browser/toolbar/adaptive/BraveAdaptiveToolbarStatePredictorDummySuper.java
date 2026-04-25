/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.adaptive;

import android.content.Context;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.ui.permissions.AndroidPermissionDelegate;

/**
 * Dummy super class for BraveAdaptiveToolbarStatePredictor that allows to call the private methods
 * of AdaptiveToolbarStatePredictor via bytecode modification.
 */
@NullMarked
public class BraveAdaptiveToolbarStatePredictorDummySuper extends AdaptiveToolbarStatePredictor {
    public BraveAdaptiveToolbarStatePredictorDummySuper(
            Context context,
            Profile profile,
            @Nullable AndroidPermissionDelegate androidPermissionDelegate,
            @Nullable AdaptiveToolbarBehavior behavior) {
        super(context, profile, androidPermissionDelegate, behavior);

        assert false : "This class usage should be removed via bytecode modification!";
    }

    public static boolean isValidSegment(@AdaptiveToolbarButtonVariant int variant) {
        assert false : "This class usage should be removed via bytecode modification!";
        return false;
    }
}
