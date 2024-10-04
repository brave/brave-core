/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class BraveSettingsIntentUtil {

    public static Intent createIntent(
            @NonNull Context context,
            @Nullable String fragmentName,
            @Nullable Bundle fragmentArgs) {
        Intent intent = SettingsIntentUtil.createIntent(context, fragmentName, fragmentArgs);
        intent.setClass(context, BraveSettingsActivity.class);
        return intent;
    }
}
