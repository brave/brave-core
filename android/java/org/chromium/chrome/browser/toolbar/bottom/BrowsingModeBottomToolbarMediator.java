/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.ThemeColorProvider.ThemeColorObserver;

/**
 * This class is responsible for reacting to events from the outside world, interacting with other
 * coordinators, running most of the business logic associated with the browsing mode bottom
 * toolbar, and updating the model accordingly.
 */
class BrowsingModeBottomToolbarMediator implements ThemeColorObserver {
    /** The dismissable parameter name of the IPH. */
    static final String DUET_IPH_TAP_TO_DISMISS_PARAM_NAME = "duet_iph_tap_to_dismiss_enabled";

    /** The model for the browsing mode bottom toolbar that holds all of its state. */
    private final BrowsingModeBottomToolbarModel mModel;

    /** A provider that notifies components when the theme color changes.*/
    private ThemeColorProvider mThemeColorProvider;

    /**
     * Build a new mediator that handles events from outside the bottom toolbar.
     * @param model The {@link BrowsingModeBottomToolbarModel} that holds all the state for the
     *              browsing mode  bottom toolbar.
     */
    BrowsingModeBottomToolbarMediator(BrowsingModeBottomToolbarModel model) {
        mModel = model;
    }

    void setThemeColorProvider(ThemeColorProvider themeColorProvider) {
        mThemeColorProvider = themeColorProvider;
        mThemeColorProvider.addThemeColorObserver(this);
    }

    /**
     * Clean up anything that needs to be when the bottom toolbar is destroyed.
     */
    void destroy() {
        if (mThemeColorProvider != null) {
            mThemeColorProvider.removeThemeColorObserver(this);
            mThemeColorProvider = null;
        }
    }

    @Override
    public void onThemeColorChanged(int primaryColor, boolean shouldAnimate) {
        mModel.set(BrowsingModeBottomToolbarModel.PRIMARY_COLOR, primaryColor);
    }
}
