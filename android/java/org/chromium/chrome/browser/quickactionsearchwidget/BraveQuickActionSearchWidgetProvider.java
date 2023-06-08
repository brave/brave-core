/*
  Copyright (c) 2023 The Brave Authors. All rights reserved.
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.quickactionsearchwidget;

import androidx.annotation.NonNull;

import org.chromium.chrome.browser.quickactionsearchwidget.QuickActionSearchWidgetProvider;

public abstract class BraveQuickActionSearchWidgetProvider extends QuickActionSearchWidgetProvider {
    public static void setWidgetEnabled(
            boolean shouldEnableQuickActionSearchWidget, boolean shouldEnableDinoVariant) {
        setWidgetComponentEnabled(QuickActionSearchWidgetProviderSearch.class, false);
        setWidgetComponentEnabled(QuickActionSearchWidgetProviderDino.class, false);
    }

    public static void setWidgetComponentEnabled(
            @NonNull Class<? extends QuickActionSearchWidgetProvider> component,
            boolean shouldEnableWidgetComponent) {
        assert false : "setWidgetComponentEnabled should be redirected to parent in bytecode!";
    }
}
