/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.custom_app_icons.settings;

import org.chromium.brave.browser.custom_app_icons.CustomAppIconsEnum;

public interface CustomAppIconsListener {
    /**
     * Called when a custom app icon is selected by the user.
     *
     * @param icon The selected custom app icon enum value
     */
    void onCustomAppIconSelected(CustomAppIconsEnum icon);
}
