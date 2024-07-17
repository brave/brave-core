/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import android.graphics.drawable.Drawable;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewStub;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.OneShotCallback;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.omaha.UpdateMenuItemHelper;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.toolbar.menu_button.BraveMenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButton;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonState;
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
     *
     * @param stub The tab switcher bottom toolbar {@link ViewStub} to inflate.
     * @param topToolbarRoot The root {@link ViewGroup} of the top toolbar.
     * @param incognitoStateProvider Notifies components when incognito mode is entered or exited.
     * @param themeColorProvider Notifies components when the theme color changes.
     * @param newTabClickListener An {@link OnClickListener} that is triggered when the new tab
     *     button is clicked.
     * @param closeTabsClickListener An {@link OnClickListener} that is triggered when the close all
     *     tabs button is clicked.
     * @param menuButtonHelper An {@link AppMenuButtonHelper} that is triggered when the menu button
     *     is clicked.
     */
    TabSwitcherBottomToolbarCoordinator(
            ViewStub stub,
            ViewGroup topToolbarRoot,
            IncognitoStateProvider incognitoStateProvider,
            ThemeColorProvider themeColorProvider,
            OnClickListener newTabClickListener,
            OnClickListener closeTabsClickListener,
            ObservableSupplier<AppMenuButtonHelper> menuButtonHelperSupplier,
            Profile profile) {
        final ViewGroup root = (ViewGroup) stub.inflate();

        View toolbar = root.findViewById(R.id.bottom_toolbar_buttons);
        ViewGroup.LayoutParams params = toolbar.getLayoutParams();
        params.height = root.getResources().getDimensionPixelOffset(R.dimen.bottom_controls_height);

        mModel = new TabSwitcherBottomToolbarModel();

        PropertyModelChangeProcessor.create(
                mModel,
                root,
                new TabSwitcherBottomToolbarViewBinder(
                        topToolbarRoot, (ViewGroup) root.getParent()));

        mMediator = new TabSwitcherBottomToolbarMediator(mModel, themeColorProvider);

        mNewTabButton = root.findViewById(R.id.tab_switcher_new_tab_button);
        Drawable background =
                ApiCompatibilityUtils.getDrawable(
                        root.getResources(), R.drawable.home_surface_search_box_background);
        background.mutate();
        mNewTabButton.setBackground(background);
        mNewTabButton.setOnClickListener(newTabClickListener);
        mNewTabButton.setIncognitoStateProvider(incognitoStateProvider);
        mNewTabButton.setThemeColorProvider(themeColorProvider);

        mMenuButton = root.findViewById(R.id.menu_button_wrapper);
        if (mMenuButton != null) {
            Supplier<MenuButtonState> menuButtonStateSupplier =
                    () -> UpdateMenuItemHelper.getInstance(profile).getUiState().buttonState;
            BraveMenuButtonCoordinator.setupPropertyModel(mMenuButton, menuButtonStateSupplier);
        }

        new OneShotCallback<>(
                menuButtonHelperSupplier,
                (menuButtonHelper) -> {
                    assert menuButtonHelper != null;
                    mMenuButton.setAppMenuButtonHelper(menuButtonHelper);
                });
    }

    /**
     * @param showOnTop Whether to show the tab switcher bottom toolbar on the top of the screen.
     */
    void showToolbarOnTop(boolean showOnTop, boolean isGridTabSwitcherEnabled) {
        mMediator.showToolbarOnTop(showOnTop, isGridTabSwitcherEnabled);
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
