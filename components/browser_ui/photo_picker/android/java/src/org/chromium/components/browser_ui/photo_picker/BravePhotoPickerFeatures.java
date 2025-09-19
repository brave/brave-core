/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.photo_picker;

import android.os.Build;

import org.chromium.build.annotations.NullMarked;

/** Provides an API for querying the status of Photo Picker features in Brave. */
@NullMarked
public class BravePhotoPickerFeatures {
    /**
     * Determines whether to launch the photo picker via ACTION_GET_CONTENT. This is the
     * Brave-specific implementation that have different behavior from the standard Chromium
     * implementation.
     *
     * @return true if ACTION_GET_CONTENT should be used, false otherwise
     */
    public static boolean launchViaActionGetContent() {
        // For Android T and above, prefer ACTION_GET_CONTENT as the default behavior
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            return true;
        }

        return PhotoPickerFeatures.launchViaActionGetContent();
    }
}
