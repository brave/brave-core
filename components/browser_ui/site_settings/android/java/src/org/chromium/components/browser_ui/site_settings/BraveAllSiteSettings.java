/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.site_settings;

import android.os.Bundle;
import android.view.View;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.settings.BottomInsetViewProvider;

/** Supplies the inset target for site lists with footer buttons. */
@NullMarked
public abstract class BraveAllSiteSettings extends BaseSiteSettingsFragment
        implements BottomInsetViewProvider {
    // These categories place footer buttons outside the preference list.
    private static final @SiteSettingsCategory.Type int[] CATEGORIES_WITH_FOOTER = {
        SiteSettingsCategory.Type.USE_STORAGE, SiteSettingsCategory.Type.ZOOM
    };

    @Override
    public @Nullable View getBottomInsetView(View fragmentView) {
        Bundle args = getArguments();
        String category = args == null ? null : args.getString(AllSiteSettings.EXTRA_CATEGORY);
        for (@SiteSettingsCategory.Type int footerCategory : CATEGORIES_WITH_FOOTER) {
            if (SiteSettingsCategory.preferenceKey(footerCategory).equals(category)) {
                return fragmentView;
            }
        }
        return fragmentView.findViewById(R.id.recycler_view);
    }
}
