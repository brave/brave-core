/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_app_icons.settings;

import org.chromium.brave.browser.custom_app_icons.CustomAppIcons.AppIconType;
import org.chromium.build.annotations.NullMarked;

/** Interface for handling custom app icon selection events. */
@NullMarked
public interface CustomAppIconsListener {
    /**
     * Called when a custom app icon is selected by the user.
     *
     * @param appIconType The selected custom app icon enum value
     */
    void onCustomAppIconSelected(@AppIconType int appIconType);
}
