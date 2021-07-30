/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import org.chromium.chrome.browser.safe_browsing.settings.BraveStandardProtectionSettingsFragment;
import org.chromium.chrome.browser.safe_browsing.settings.StandardProtectionSettingsFragment;

public class BraveSettingsLauncherImpl extends SettingsLauncherImpl {
    public BraveSettingsLauncherImpl() {
        super();
    }

    @Override
    public void launchSettingsActivity(Context context,
            @Nullable Class<? extends Fragment> fragment, @Nullable Bundle fragmentArgs) {
        if (fragment != null) {
            // Substitute with our version of class
            if (fragment.equals(StandardProtectionSettingsFragment.class)) {
                fragment = BraveStandardProtectionSettingsFragment.class;
            }
        }
        super.launchSettingsActivity(context, fragment, fragmentArgs);
    }

    @Override
    public Intent createSettingsActivityIntent(
            Context context, @Nullable String fragmentName, @Nullable Bundle fragmentArgs) {
        Intent intent = super.createSettingsActivityIntent(context, fragmentName, fragmentArgs);
        intent.setClass(context, BraveSettingsActivity.class);
        return intent;
    }
}
