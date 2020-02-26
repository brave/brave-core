/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.download;

import android.os.Bundle;

import androidx.annotation.Nullable;

import org.chromium.chrome.R;

public class BraveDownloadPreferences extends DownloadPreferences {
    private static final String PREF_PREFETCHING_ENABLED = "prefetching_enabled";

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String s) {
        super.onCreatePreferences(savedInstanceState, s);
        getPreferenceScreen().removePreference(findPreference(PREF_PREFETCHING_ENABLED));
    }
}
