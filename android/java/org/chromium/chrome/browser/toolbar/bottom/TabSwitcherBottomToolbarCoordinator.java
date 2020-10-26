/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.graphics.drawable.Drawable;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewStub;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.toolbar.TabCountProvider;
import org.chromium.chrome.browser.toolbar.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButton;
import org.chromium.chrome.browser.ui.appmenu.AppMenuButtonHelper;
import org.chromium.ui.modelutil.PropertyModelChangeProcessor;

/**
 * The coordinator for the tab switcher mode bottom toolbar. This class handles all interactions
 * that the tab switcher bottom toolbar has with the outside world.
 * TODO(crbug.com/1036474): This coordinator is not used currently and can be removed if the final
 *                          duet design doesn't need a stand-alone toolbar in tab switcher mode.
 */
public class TabSwitcherBottomToolbarCoordinator {
    /** The mediator that handles events from outside the tab switcher bottom toolbar. */
    private final TabSwitcherBottomToolbarMediator mMediator;

    /** The new tab button that lives in the tab switcher bottom toolbar. */
    private final BottomToolbarNewTabButton mNewTabButton;

    /** The menu button that lives in the tab switcher bottom toolbar. */
    private final MenuButton mMenuButton;

    /** The model for the tab switcher bottom toolbar that holds all of its state. */
    private final TabSwitcherBottomToolbarModel mModel;

    /**
     * Build the coordinator that manages the tab switcher bottom toolbar.
     * @param stub The tab switcher bottom toolbar {@link ViewStub} to inflate.
     * @param topToolbarRoot The root {@link ViewGroup} of the top toolbar.
     * @param incognitoStateProvider Notifies components when incognito mode is entered or exited.
     * @param themeColorProvider Notifies components when the theme color changes.
     * @param newTabClickListener An {@link OnClickListener} that is triggered when the
     *                            new tab button is clicked.
     * @param closeTabsClickListener An {@link OnClickListener} that is triggered when the
     *                               close all tabs button is clicked.
     * @param menuButtonHelper An {@link AppMenuButtonHelper} that is triggered when the
     *                         menu button is clicked.
     * @param tabCountProvider Updates the tab count number in the tab switcher button and in the
     *                         incognito toggle tab layout.
     */
    TabSwitcherBottomToolbarCoordinator(ViewStub stub, ViewGroup topToolbarRoot,
            IncognitoStateProvider incognitoStateProvider, ThemeColorProvider themeColorProvider,
            OnClickListener newTabClickListener, OnClickListener closeTabsClickListener,
            AppMenuButtonHelper menuButtonHelper, TabCountProvider tabCountProvider) {
        final ViewGroup root = (ViewGroup) stub.inflate();

        View toolbar = root.findViewById(R.id.bottom_toolbar_buttons);
        ViewGroup.LayoutParams params = toolbar.getLayoutParams();
        params.height = root.getResources().getDimensionPixelOffset(R.dimen.bottom_controls_height);

        mModel = new TabSwitcherBottomToolbarModel();

        PropertyModelChangeProcessor.create(mModel, root,
                new TabSwitcherBottomToolbarViewBinder(
                        topToolbarRoot, (ViewGroup) root.getParent()));

        mMediator = new TabSwitcherBottomToolbarMediator(mModel, themeColorProvider);

        mNewTabButton = root.findViewById(R.id.tab_switcher_new_tab_button);
        Drawable background =
                ApiCompatibilityUtils.getDrawable(root.getResources(), R.drawable.ntp_search_box);
        background.mutate();
        mNewTabButton.setBackground(background);
        mNewTabButton.setOnClickListener(newTabClickListener);
        mNewTabButton.setIncognitoStateProvider(incognitoStateProvider);
        mNewTabButton.setThemeColorProvider(themeColorProvider);

        assert menuButtonHelper != null;
        mMenuButton = root.findViewById(R.id.menu_button_wrapper);
        mMenuButton.setAppMenuButtonHelper(menuButtonHelper);
    }

    /**
     * @param showOnTop Whether to show the tab switcher bottom toolbar on the top of the screen.
     */
    void showToolbarOnTop(boolean showOnTop) {
        mMediator.showToolbarOnTop(showOnTop);
    }

    /**
     * @param visible Whether to hide the tab switcher bottom toolbar
     */
    void setVisible(boolean visible) {
        mModel.set(TabSwitcherBottomToolbarModel.IS_VISIBLE, visible);
    }

    /**
     * Clean up any state when the bottom toolbar is destroyed.
     */
    public void destroy() {
        mMediator.destroy();
        mNewTabButton.destroy();
    }
}
