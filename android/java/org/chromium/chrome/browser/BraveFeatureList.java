/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

@JNINamespace("chrome::android")
public abstract class BraveFeatureList {
    public static final String BRAVE_REWARDS = "BraveRewards";
    public static final String USE_DEV_UPDATER_URL = "UseDevUpdaterUrl";
    public static final String FORCE_WEB_CONTENTS_DARK_MODE = "WebContentsForceDark";
    public static final String ENABLE_FORCE_DARK = "enable-force-dark";
    public static final String ENABLE_FORCE_DARK_DISABLED_VALUE = "0";
    public static final String ENABLE_TAB_GROUPS = "enable-tab-groups";
    public static final String ENABLE_TAB_GROUPS_DISABLED_VALUE = "2";
    public static final String ENABLE_TAB_GRID = "enable-tab-grid-layout";
    public static final String ENABLE_TAB_GRID_DISABLED_VALUE = "7";

    public static void enableFeature(String featureName, boolean enabled, String disabledValue) {
        BraveFeatureListJni.get().enableFeature(featureName, enabled, disabledValue);
    }

    @NativeMethods
    interface Natives {
        void enableFeature(String featureName, boolean enabled, String disabledValue);
    }
}
