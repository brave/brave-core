/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.Nullable;

public class BraveSettingsIntentUtil {
    public static Intent createIntent(
            Context context, @Nullable String fragmentName, @Nullable Bundle fragmentArgs) {
        return createIntent(context, fragmentName, fragmentArgs, /* addToBackStack= */ false);
    }

    public static Intent createIntent(
            Context context,
            @Nullable String fragmentName,
            @Nullable Bundle fragmentArgs,
            boolean addToBackStack) {
        return createIntent(context, fragmentName, fragmentArgs, addToBackStack, /* tag= */ null);
    }

    public static Intent createIntent(
            Context context,
            @Nullable String fragmentName,
            @Nullable Bundle fragmentArgs,
            boolean addToBackStack,
            @Nullable String tag) {
        Intent intent =
                SettingsIntentUtil.createIntent(
                        context, fragmentName, fragmentArgs, addToBackStack, tag);
        intent.setClass(context, BraveSettingsActivity.class);
        return intent;
    }
}
