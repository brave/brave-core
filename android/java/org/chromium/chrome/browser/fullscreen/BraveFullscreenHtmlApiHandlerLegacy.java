/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.fullscreen;

import android.app.Activity;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;

public class BraveFullscreenHtmlApiHandlerLegacy extends FullscreenHtmlApiHandlerLegacy {

    /**
     * Constructs the legacy handler that will manage the UI transitions from the HTML fullscreen
     * API.
     *
     * @param activity The activity that supports fullscreen.
     * @param areControlsHidden Supplier of a flag indicating if browser controls are hidden.
     * @param exitFullscreenOnStop Whether fullscreen mode should exit on stop - should be true for
     *     Activities that are not always fullscreen.
     */
    public BraveFullscreenHtmlApiHandlerLegacy(
            Activity activity,
            MonotonicObservableSupplier<Boolean> areControlsHidden,
            boolean exitFullscreenOnStop,
            MultiWindowModeStateDispatcher multiWindowDispatcher) {
        super(activity, areControlsHidden, exitFullscreenOnStop, multiWindowDispatcher);
    }

    @Override
    public void exitPersistentFullscreenMode() {
        if (!mActivity.isInPictureInPictureMode()
                || BraveFullscreenHtmlApiHandlerBase.class.cast(this).mTabHiddenByChangedTabs) {
            super.exitPersistentFullscreenMode();
        }
    }
}
