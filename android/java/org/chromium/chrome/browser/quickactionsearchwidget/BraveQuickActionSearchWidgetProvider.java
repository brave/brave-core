/*
  Copyright (c) 2022 The Brave Authors. All rights reserved.
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this file,
  You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.quickactionsearchwidget;

import org.chromium.chrome.browser.quickactionsearchwidget.QuickActionSearchWidgetProvider;

public abstract class BraveQuickActionSearchWidgetProvider extends QuickActionSearchWidgetProvider {
    public static void setWidgetEnabled(
            boolean shouldEnableQuickActionSearchWidget, boolean shouldEnableDinoVariant) {
        // We don't need to do anything here as in QuickActionSearchWidgetProvider we only set dino
        // and quick search widgets.
    }
}
