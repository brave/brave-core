/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import org.chromium.chrome.browser.browsing_data.BraveClearBrowsingDataFragment;
import org.chromium.chrome.browser.browsing_data.ClearBrowsingDataFragment;
import org.chromium.chrome.browser.download.settings.BraveDownloadSettings;
import org.chromium.chrome.browser.download.settings.DownloadSettings;
import org.chromium.chrome.browser.safe_browsing.settings.BraveStandardProtectionSettingsFragment;
import org.chromium.chrome.browser.safe_browsing.settings.StandardProtectionSettingsFragment;
import org.chromium.chrome.browser.tasks.tab_management.BraveTabUiFeatureUtilities;
import org.chromium.chrome.browser.tasks.tab_management.TabsSettings;

public class BraveSettingsLauncherImpl extends SettingsNavigationImpl {
    public BraveSettingsLauncherImpl() {
        super();
    }

    // This is the only overload that every startSettings() and createSettingsIntent() variant in
    // SettingsNavigationImpl funnels into, so overriding it applies the Brave fragment substitution
    // to all entry points, including the ones that pass a @SettingsFragment enum value instead of a
    // fragment class (for example Quick Delete's "More options" and the History page's "Delete
    // browsing data").
    @Override
    public Intent createSettingsIntent(
            Context context,
            @Nullable Class<? extends Fragment> fragment,
            @Nullable Bundle fragmentArgs,
            boolean addToBackStack,
            @Nullable String tag) {
        // Substitute before calling super: SettingsIntentUtil decides whether the fragment is
        // standalone by reflecting on the fragment name, so it has to see the Brave class.
        if (fragment != null) {
            fragment = substituteFragment(fragment);
        }
        Intent intent =
                super.createSettingsIntent(context, fragment, fragmentArgs, addToBackStack, tag);
        intent.setClass(context, BraveSettingsActivity.class);
        return intent;
    }

    private Class<? extends Fragment> substituteFragment(Class<? extends Fragment> fragment) {
        // Substitute with our version of class.
        if (fragment.equals(StandardProtectionSettingsFragment.class)) {
            return BraveStandardProtectionSettingsFragment.class;
        } else if (fragment.equals(DownloadSettings.class)) {
            return BraveDownloadSettings.class;
        } else if (fragment.equals(ClearBrowsingDataFragment.class)) {
            return BraveClearBrowsingDataFragment.class;
        } else if (fragment.equals(TabsSettings.class)
                && BraveTabUiFeatureUtilities.isBraveAndroidTabGroupsSettingsFeatureEnabled()) {
            return BraveTabsAndTabGroupsSettings.class;
        } else if (fragment.equals(BraveTabsAndTabGroupsSettings.class)
                && !BraveTabUiFeatureUtilities.isBraveAndroidTabGroupsSettingsFeatureEnabled()) {
            return TabsSettings.class;
        }
        return fragment;
    }
}
