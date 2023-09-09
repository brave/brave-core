/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.tasks.tab_groups;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.state.CriticalPersistedTabData;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.tabmodel.TabModelFilter;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;

/**
 * Brave's super class for {@link TabGroupModelFilter}
 */
public abstract class BraveTabGroupModelFilter extends TabModelFilter {
    /**
     * This variable will be used instead of {@link
     * TabGroupModelFilter}'s variable, that will be deleted in bytecode.
     */
    protected boolean mIsResetting;

    public BraveTabGroupModelFilter(TabModel tabModel) {
        super(tabModel);
    }

    /**
     * Call from {@link TabGroupModelFilter} will be redirected here via bytrcode.
     */
    public int getParentId(Tab tab) {
        if (isTabModelRestored() && !mIsResetting
                && SharedPreferencesManager.getInstance().readBoolean(
                        BravePreferenceKeys.BRAVE_TAB_GROUPS_ENABLED, true)) {
            Tab parentTab = TabModelUtils.getTabById(
                    getTabModel(), CriticalPersistedTabData.from(tab).getParentId());
            if (parentTab != null) {
                return (int) BraveReflectionUtil.InvokeMethod(
                        TabGroupModelFilter.class, this, "getRootId", Tab.class, parentTab);
            }
        }
        // Otherwise just call parent.
        return (int) BraveReflectionUtil.InvokeMethod(
                TabGroupModelFilter.class, this, "getParentId", Tab.class, tab);
    }
}
