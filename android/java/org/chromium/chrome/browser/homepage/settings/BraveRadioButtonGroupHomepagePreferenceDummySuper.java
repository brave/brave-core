/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.homepage.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.RadioGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.homepage.settings.RadioButtonGroupHomepagePreference.PreferenceValues;
import org.chromium.components.browser_ui.settings.ContainedRadioButtonGroupPreference;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithEditText;

/**
 * This is dummy super class for {@link BraveRadioButtonGroupHomepagePreference}. Used to make code
 * compilable before bytecode manipulations.
 */
@NullMarked
public class BraveRadioButtonGroupHomepagePreferenceDummySuper
        extends ContainedRadioButtonGroupPreference {
    public BraveRadioButtonGroupHomepagePreferenceDummySuper(Context context, AttributeSet attrs) {
        super(context, attrs);

        assert false : "This dummy class should be replaced in bytecode!";
    }

    public void onCheckedChanged(RadioGroup group, int checkedId) {
        assert false : "This dummy class should be replaced in bytecode!";
    }

    @Nullable
    PreferenceValues getPreferenceValue() {
        assert false : "This dummy class should be replaced in bytecode!";
        return null;
    }

    void setupPreferenceValues(@NonNull PreferenceValues value) {
        assert false : "This dummy class should be replaced in bytecode!";
    }

    public void onTextChanged(CharSequence newText) {
        assert false : "This dummy class should be replaced in bytecode!";
    }

    @VisibleForTesting
    @Nullable
    RadioButtonWithEditText getCustomUriRadioButton() {
        assert false : "This dummy class should be replaced in bytecode!";
        return null;
    }

    @VisibleForTesting
    @Nullable
    RadioButtonWithDescription getChromeNtpRadioButton() {
        assert false : "This dummy class should be replaced in bytecode!";
        return null;
    }
}
