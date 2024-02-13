/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.customtabs.features.partialcustomtab;

import android.app.Activity;

import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.browserservices.intents.BrowserServicesIntentDataProvider;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.widget.TouchEventProvider;
import org.chromium.ui.base.PageTransition;
import org.chromium.url.GURL;

public class BravePartialCustomTabBottomSheetStrategy extends PartialCustomTabBottomSheetStrategy {

    public BravePartialCustomTabBottomSheetStrategy(
            Activity activity,
            BrowserServicesIntentDataProvider intentData,
            Supplier<TouchEventProvider> touchEventProvider,
            Supplier<Tab> tab,
            OnResizedCallback onResizedCallback,
            OnActivityLayoutCallback onActivityLayoutCallback,
            ActivityLifecycleDispatcher lifecycleDispatcher,
            FullscreenManager fullscreenManager,
            boolean isTablet,
            boolean startMaximized,
            PartialCustomTabHandleStrategyFactory handleStrategyFactory) {
        super(
                activity,
                intentData,
                touchEventProvider,
                tab,
                onResizedCallback,
                onActivityLayoutCallback,
                lifecycleDispatcher,
                fullscreenManager,
                isTablet,
                startMaximized,
                handleStrategyFactory);
    }

    public boolean mStopShowingSpinner;
    public Supplier<Tab> mTab;

    @Override
    public void onDragStart(int y) {
        super.onDragStart(y);
        if (!mTab.hasValue()) {
            return;
        }
        GURL url = mTab.get().getUrl();
        if (url.getScheme().equals("chrome-untrusted")
                && url.getHost().equals("chat")
                && TabUtils.getTransition(mTab.get()) == PageTransition.FROM_API) {
            mStopShowingSpinner = true;
        }
    }

    public static Class<OnResizedCallback> getOnResizedCallbackClass() {
        return OnResizedCallback.class;
    }

    public static Class<OnActivityLayoutCallback> getOnActivityLayoutCallbackClass() {
        return OnActivityLayoutCallback.class;
    }
}
