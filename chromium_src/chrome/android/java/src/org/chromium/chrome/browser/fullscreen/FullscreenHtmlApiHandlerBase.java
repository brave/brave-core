// Copyright 2026 The Brave Authors
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.fullscreen;

import android.app.Activity;

import org.chromium.base.ActivityState;
import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.chrome.browser.multiwindow.MultiWindowModeStateDispatcher;

public class FullscreenHtmlApiHandlerBase
        extends org.chromium.chrome.browser.fullscreen.FullscreenHtmlApiHandlerBase {

    public FullscreenHtmlApiHandlerBase(
            Activity activity,
            NonNullObservableSupplier<Boolean> areControlsHidden,
            boolean exitFullscreenOnStop,
            MultiWindowModeStateDispatcher multiWindowDispatcher) {
        super(activity, areControlsHidden, exitFullscreenOnStop, multiWindowDispatcher);
    }

    @Override
    public void onActivityStateChange(Activity activity, int newState) {
        // BRAVE FIX: keep fullscreen when Home is pressed during video
        if (newState == ActivityState.STOPPED && mExitFullscreenOnStop) {
            if (mTabInFullscreen != null && mTabInFullscreen.isPlayingMedia()) {
                // Don't exit — preserve fullscreen for PIP/background play
                return;
            }
        }
        super.onActivityStateChange(activity, newState);
    }
}