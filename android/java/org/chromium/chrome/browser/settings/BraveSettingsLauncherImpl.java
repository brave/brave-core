/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import org.chromium.chrome.browser.browsing_data.BraveClearBrowsingDataFragmentAdvanced;
import org.chromium.chrome.browser.browsing_data.ClearBrowsingDataFragmentAdvanced;
import org.chromium.chrome.browser.download.settings.BraveDownloadSettings;
import org.chromium.chrome.browser.download.settings.DownloadSettings;
import org.chromium.chrome.browser.safe_browsing.settings.BraveStandardProtectionSettingsFragment;
import org.chromium.chrome.browser.safe_browsing.settings.StandardProtectionSettingsFragment;

public class BraveSettingsLauncherImpl extends SettingsNavigationImpl {
    public BraveSettingsLauncherImpl() {
        super();
    }

    @Override
    public void startSettings(
            Context context,
            @Nullable Class<? extends Fragment> fragment,
            @Nullable Bundle fragmentArgs) {
        if (fragment != null) {
            // Substitute with our version of class
            if (fragment.equals(StandardProtectionSettingsFragment.class)) {
                fragment = BraveStandardProtectionSettingsFragment.class;
            } else if (fragment.equals(DownloadSettings.class)) {
                fragment = BraveDownloadSettings.class;
            } else if (fragment.equals(ClearBrowsingDataFragmentAdvanced.class)) {
                fragment = BraveClearBrowsingDataFragmentAdvanced.class;
            }
        }
        super.startSettings(context, fragment, fragmentArgs);
    }

    @Override
    public Intent createSettingsIntent(
            Context context,
            @Nullable Class<? extends Fragment> fragment,
            @Nullable Bundle fragmentArgs) {
        Intent intent = super.createSettingsIntent(context, fragment, fragmentArgs);
        intent.setClass(context, BraveSettingsActivity.class);
        if (!(context instanceof Activity)) {
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        }
        if (fragment != null) {
            intent.putExtra(SettingsActivity.EXTRA_SHOW_FRAGMENT, fragment.getName());
        }
        if (fragmentArgs != null) {
            intent.putExtra(SettingsActivity.EXTRA_SHOW_FRAGMENT_ARGUMENTS, fragmentArgs);
        }
        return intent;
    }
}
