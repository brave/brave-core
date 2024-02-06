/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;

import androidx.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.brave_leo.BraveLeoMojomHelper;
import org.chromium.chrome.browser.brave_leo.BraveLeoPrefUtils;
import org.chromium.components.browser_ui.settings.SettingsUtils;

public class BraveLeoDefaultModelPreferences extends BravePreferenceFragment
        implements BraveLeoRadioButtonGroupDefaultModelPreference.RadioButtonsDelegate {
    private static final String PREF_DEFAULT_MODEL_GROUP = "default_model";
    private BraveLeoRadioButtonGroupDefaultModelPreference mRadioButtons;

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_leo_default_model_preferences);
        getActivity().setTitle(R.string.leo_default_model_pref_screen);
        mRadioButtons =
                (BraveLeoRadioButtonGroupDefaultModelPreference)
                        findPreference(PREF_DEFAULT_MODEL_GROUP);
        mRadioButtons.initialize(this);
    }

    @Override
    public void fetchModels() {
        BraveLeoMojomHelper.getInstance(getProfile())
                .getModels(
                        (models -> {
                            mRadioButtons.initializeModels(
                                    models, BraveLeoPrefUtils.getDefaultModel());
                        }));
    }

    @Override
    public void setDefaultModel(String key) {
        BraveLeoPrefUtils.setDefaultModel(key);
    }
}
