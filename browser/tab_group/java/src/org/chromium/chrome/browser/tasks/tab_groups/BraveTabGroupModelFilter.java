/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.tasks.tab_groups;

import androidx.annotation.NonNull;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.Token;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelFilter;

/** Brave's super class for {@link TabGroupModelFilter} */
public abstract class BraveTabGroupModelFilter extends TabModelFilter {
    /**
     * This variable will be used instead of {@link TabGroupModelFilter}'s variable, that will be
     * deleted in bytecode.
     */
    protected boolean mIsResetting;

    public BraveTabGroupModelFilter(TabModel tabModel) {
        super(tabModel);
    }

    /** Call from {@link TabGroupModelFilter} will be redirected here via bytrcode. */
    public boolean shouldUseParentIds(Tab tab) {
        if (linkClicked(tab.getLaunchType())
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED, true)
                && isTabModelRestored()
                && !mIsResetting) {
            return true;
        }
        // Otherwise just call parent.
        return (boolean)
                BraveReflectionUtil.invokeMethod(
                        TabGroupModelFilter.class, this, "shouldUseParentIds", Tab.class, tab);
    }

    /** Determine if a launch type is the result of linked being clicked. */
    private boolean linkClicked(@TabLaunchType int type) {
        return type == TabLaunchType.FROM_LINK || type == TabLaunchType.FROM_LONGPRESS_FOREGROUND;
    }

    private static Token getOrCreateTabGroupId(@NonNull Tab tab) {
        return (Token)
                BraveReflectionUtil.invokeMethod(
                        TabGroupModelFilter.class, null, "getOrCreateTabGroupId", Tab.class, tab);
    }
}
