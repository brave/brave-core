// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_manager.settings;

import android.content.Context;
import android.util.AttributeSet;

import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;

@NullMarked
public class BravePasswordsPreference extends PasswordsPreference {
    public BravePasswordsPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void setUpPostDeprecationWarning(PreferenceViewHolder holder) {}
}
