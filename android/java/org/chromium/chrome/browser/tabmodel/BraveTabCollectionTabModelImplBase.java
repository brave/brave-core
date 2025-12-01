/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.tabmodel;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabLaunchType;

/** Brave's super class for {@link TabCollectionTabModelImpl} */
@NullMarked
public abstract class BraveTabCollectionTabModelImplBase extends TabModelJniBridge {
    public BraveTabCollectionTabModelImplBase(Profile profile) {
        super(profile);
    }

    /** Call from {@link TabCollectionTabModelImpl} will be redirected here via bytecode. */
    @SuppressWarnings("UnusedMethod")
    protected boolean shouldGroupWithParent(Tab tab, @Nullable Tab parentTab) {
        // We can't group with a parent if it's null.
        if (parentTab == null) {
            return false;
        }
        if (linkClicked(tab.getLaunchType())
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(
                                BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED,
                                ChromeSharedPreferences.getInstance()
                                        .readBoolean(
                                                BravePreferenceKeys
                                                        .BRAVE_TAB_GROUPS_ENABLED_DEFAULT_VALUE,
                                                true))
                && isTabModelRestored()) {
            return true;
        }
        // Otherwise just call parent.
        @Nullable Boolean shouldGroupWithParent =
                (Boolean)
                        BraveReflectionUtil.invokeMethod(
                                TabCollectionTabModelImpl.class,
                                this,
                                "shouldGroupWithParent",
                                Tab.class,
                                tab,
                                Tab.class,
                                parentTab);
        return shouldGroupWithParent != null && shouldGroupWithParent;
    }

    /** Determine if a launch type is the result of linked being clicked. */
    private boolean linkClicked(@TabLaunchType int type) {
        return type == TabLaunchType.FROM_LINK || type == TabLaunchType.FROM_LONGPRESS_FOREGROUND;
    }

    private boolean isTabModelRestored() {
        @Nullable Boolean isRestored =
                (Boolean)
                        BraveReflectionUtil.invokeMethod(
                                TabCollectionTabModelImpl.class, this, "isTabModelRestored");
        return isRestored != null && isRestored;
    }
}
