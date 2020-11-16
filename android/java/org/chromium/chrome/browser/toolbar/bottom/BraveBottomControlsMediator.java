/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.ui.modelutil.PropertyModel;

class BraveBottomControlsMediator extends BottomControlsMediator {
    private final PropertyModel mModel;

    BraveBottomControlsMediator(PropertyModel model, BrowserControlsSizer controlsSizer,
            FullscreenManager fullscreenManager, int bottomControlsHeight) {
        super(model, controlsSizer, fullscreenManager, bottomControlsHeight);

        mModel = model;
    }

    public void setCompositedViewVisibile(boolean visible) {
        mModel.set(BottomControlsProperties.COMPOSITED_VIEW_VISIBLE, visible);
    }
}
