/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.site_settings;

import android.os.Bundle;
import android.view.MenuItem;

import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.Log;
import org.chromium.build.annotations.UsedByReflection;
import org.chromium.components.content_settings.ContentSettingsType;
import org.chromium.content_public.browser.BrowserContextHandle;

@UsedByReflection("brave_site_settings_preferences.xml")
public class BraveSingleCategorySettings
        extends SiteSettingsPreferenceFragment implements AddExceptionPreference.SiteAddedCallback {
    private static final String ADD_EXCEPTION_KEY = "add_exception";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {}

    public String getAddExceptionDialogMessage() {
        BrowserContextHandle browserContextHandle =
                getSiteSettingsDelegate().getBrowserContextHandle();
        int resource = 0;
        SiteSettingsCategory mCategory = (SiteSettingsCategory) BraveReflectionUtil.getField(
                SingleCategorySettings.class, "mCategory", this);

        if (mCategory.getType() == SiteSettingsCategory.Type.AUTOPLAY) {
            resource = R.string.website_settings_add_site_description_autoplay;
        } else {
            return (String) BraveReflectionUtil.InvokeMethod(
                    SingleCategorySettings.class, this, "getAddExceptionDialogMessage");
        }
        assert resource > 0;
        return getString(resource);
    }

    public void resetList() {
        BraveReflectionUtil.InvokeMethod(SingleCategorySettings.class, this, "resetList");
        BrowserContextHandle browserContextHandle =
                getSiteSettingsDelegate().getBrowserContextHandle();
        boolean exception = false;
        SiteSettingsCategory mCategory = (SiteSettingsCategory) BraveReflectionUtil.getField(
                SingleCategorySettings.class, "mCategory", this);

        if (mCategory.getType() == SiteSettingsCategory.Type.AUTOPLAY) {
            exception = true;
        }
        if (exception) {
            getPreferenceScreen().addPreference(
                    new AddExceptionPreference(getPreferenceManager().getContext(),
                            ADD_EXCEPTION_KEY, getAddExceptionDialogMessage(), mCategory, this));
        }
    }

    @Override
    public void onAddSite(String primaryPattern, String secondaryPattern) {
        assert (false);
    }
}
