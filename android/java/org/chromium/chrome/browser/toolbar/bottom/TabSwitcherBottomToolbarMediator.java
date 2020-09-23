// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar.bottom;

import org.chromium.chrome.browser.ThemeColorProvider;
import org.chromium.chrome.browser.ThemeColorProvider.ThemeColorObserver;
import org.chromium.chrome.browser.tasks.tab_management.TabUiFeatureUtilities;

/**
 * This class is responsible for reacting to events from the outside world, interacting with other
 * coordinators, running most of the business logic associated with the tab switcher bottom toolbar,
 * and updating the model accordingly.
 */
class TabSwitcherBottomToolbarMediator implements ThemeColorObserver {
    /** The model for the tab switcher bottom toolbar that holds all of its state. */
    private final TabSwitcherBottomToolbarModel mModel;

    /** A provider that notifies components when the theme color changes.*/
    private final ThemeColorProvider mThemeColorProvider;

    /**
     * Build a new mediator that handles events from outside the tab switcher bottom toolbar.
     * @param model The {@link TabSwitcherBottomToolbarModel} that holds all the state for the
     *              tab switcher bottom toolbar.
     * @param themeColorProvider Notifies components when the theme color changes.
     */
    TabSwitcherBottomToolbarMediator(
            TabSwitcherBottomToolbarModel model, ThemeColorProvider themeColorProvider) {
        mModel = model;

        mThemeColorProvider = themeColorProvider;
        mThemeColorProvider.addThemeColorObserver(this);
    }

    /**
     * @param showOnTop Whether to show the tab switcher bottom toolbar on the top of the screen.
     */
    void showToolbarOnTop(boolean showOnTop) {
        // TODO(crbug.com/1012014): Resolve how to manage the toolbar position in tab switcher in
        // landscape mode. Probably remove code about showing bottom toolbar on top.
        // When GridTabSwitcher is enabled, show the original top toolbar instead of showing the
        // bottom toolbar on top.
        mModel.set(TabSwitcherBottomToolbarModel.SHOW_ON_TOP,
                showOnTop && !TabUiFeatureUtilities.isGridTabSwitcherEnabled());
    }

    /**
     * Clean up anything that needs to be when the tab switcher bottom toolbar is destroyed.
     */
    void destroy() {
        mThemeColorProvider.removeThemeColorObserver(this);
    }

    @Override
    public void onThemeColorChanged(int primaryColor, boolean shouldAnimate) {
        mModel.set(TabSwitcherBottomToolbarModel.PRIMARY_COLOR, primaryColor);
    }
}
