/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.bottombar;

import android.view.ViewGroup;

import org.chromium.base.Callback;
import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.base.supplier.NullableObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.ui.actions.ActionButtonBinder;
import org.chromium.chrome.browser.ui.actions.ActionId;
import org.chromium.chrome.browser.ui.actions.ActionRegistry;
import org.chromium.chrome.browser.ui.bottombar.BottomBarButtonManager.ActionConfig;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.ArrayList;
import java.util.List;

/**
 * Gives the bottom bar the buttons Brave's own bottom navigation controls used to show: home,
 * bookmark, search, tab switcher and app menu.
 *
 * <p>Brave's bottom navigation controls are off whenever the Android bottom bar flag is on, so the
 * bottom bar takes over their button set. Three slots change:
 *
 * <ul>
 *   <li>The centre slot holds the search accelerator instead of the new tab button.
 *   <li>The container upstream shares between Glic and AI Mode holds the bookmark button.
 *   <li>The home slot is shared with the new tab button, which stands in for the home button while
 *       the homepage is disabled - as Brave's own home button did.
 * </ul>
 *
 * <p>The tab switcher and app menu slots stay upstream's, so the flag's variation params keep
 * working on them. What the added buttons look like and do is filled in by {@code
 * BraveBottomBarActionCoordinator}.
 */
@NullMarked
public class BraveBottomBarCoordinator extends BottomBarCoordinator {
    private final NonNullObservableSupplier<Boolean> mHomepageEnabledSupplier;
    private final Callback<Boolean> mHomepageEnabledObserver = this::onHomepageEnabledChanged;

    public BraveBottomBarCoordinator(
            ViewGroup parent,
            ActionRegistry actionRegistry,
            ThemeColorProvider themeColorProvider,
            NullableObservableSupplier<Tab> tabSupplier,
            NonNullObservableSupplier<Boolean> homepageEnabledSupplier,
            BottomBarMediator.VisibilityDelegate visibilityDelegate,
            NullableObservableSupplier<Profile> profileSupplier,
            OneshotSupplier<String> countrySupplier,
            NonNullObservableSupplier<Boolean> omniboxFocusStateSupplier,
            NonNullObservableSupplier<ModalDialogManager> modalDialogManagerSupplier,
            LayoutStateProvider layoutStateProvider) {
        super(
                parent,
                actionRegistry,
                themeColorProvider,
                tabSupplier,
                homepageEnabledSupplier,
                visibilityDelegate,
                profileSupplier,
                countrySupplier,
                omniboxFocusStateSupplier,
                modalDialogManagerSupplier,
                layoutStateProvider);

        mHomepageEnabledSupplier = homepageEnabledSupplier;
        // Only registered when the home slot is in the bottom bar at all - the 1C variations keep
        // the home button in the top toolbar, and then there is no new tab button to stand in for
        // it either.
        if (BottomBarConfigUtils.shouldIncludeHomeButtonIfEnabled()) {
            mHomepageEnabledSupplier.addSyncObserverAndCallIfNonNull(mHomepageEnabledObserver);
        }
    }

    /**
     * Rearranges upstream's button set into Brave's.
     *
     * <p>Glic and AI Mode stay registered - and stay hidden, as neither is eligible in Brave - so
     * that {@link BottomBarMediator} keeps resolving them exactly as upstream does.
     *
     * <p>Called from the {@link BottomBarCoordinator} constructor, so it must not touch state of
     * this class.
     */
    @Override
    protected List<ActionConfig> createActionConfigs(
            BottomBarView view, boolean shouldIncludeHomeButton) {
        List<ActionConfig> configs =
                new ArrayList<>(super.createActionConfigs(view, shouldIncludeHomeButton));

        // The search accelerator replaces the new tab button in the centre slot, keeping the
        // visibility key upstream's view binder maps to that container.
        int centerIndex = indexOfAction(configs, ActionId.NEW_TAB);
        BottomBarButtonContainer centerContainer = configs.get(centerIndex).container;
        configs.set(
                centerIndex,
                new ActionConfig(
                        ActionId.BRAVE_SEARCH,
                        centerContainer,
                        ActionButtonBinder::bind,
                        BottomBarProperties.IS_NEW_TAB_BUTTON_VISIBLE,
                        /* initiallyVisible= */ true));

        // The bookmark button sits between the home slot and the centre slot, so it has to be
        // registered ahead of the centre button to be scored to the left of it.
        BottomBarButtonContainer bookmarkContainer =
                view.getContainerForAction(ActionId.BRAVE_BOOKMARK);
        assert bookmarkContainer != null : "Bookmark button container not found";
        configs.add(
                centerIndex,
                new ActionConfig(
                        ActionId.BRAVE_BOOKMARK,
                        bookmarkContainer,
                        ActionButtonBinder::bind,
                        BottomBarProperties.IS_EXTRA_BUTTON_VISIBLE,
                        /* initiallyVisible= */ true));

        // The new tab button joins the home button in the home slot, sharing its visibility key so
        // that the slot is shown whenever either of the two is. onHomepageEnabledChanged() picks
        // which one; it starts hidden because upstream's mediator starts the home button visible.
        if (shouldIncludeHomeButton) {
            int homeIndex = indexOfAction(configs, ActionId.HOME_BUTTON);
            configs.add(
                    homeIndex + 1,
                    new ActionConfig(
                            ActionId.NEW_TAB,
                            configs.get(homeIndex).container,
                            ActionButtonBinder::bind,
                            BottomBarProperties.IS_HOME_BUTTON_VISIBLE,
                            /* initiallyVisible= */ false));
        }

        return configs;
    }

    /** Brave's bookmark button owns the container the promo dialog advertises. */
    @Override
    public boolean maybeShowPromoDialog(Profile profile) {
        return false;
    }

    @Override
    public void destroy() {
        mHomepageEnabledSupplier.removeObserver(mHomepageEnabledObserver);
        super.destroy();
    }

    /**
     * Shows the new tab button in place of the home button while the homepage is disabled. Upstream
     * drives the home button off the same supplier, so only the new tab button is set here.
     */
    private void onHomepageEnabledChanged(boolean homepageEnabled) {
        mButtonManager.setButtonVisibility(ActionId.NEW_TAB, !homepageEnabled);
    }

    private static int indexOfAction(List<ActionConfig> configs, @ActionId int actionId) {
        for (int i = 0; i < configs.size(); i++) {
            if (configs.get(i).actionId == actionId) return i;
        }
        assert false : "Action not found in configs: " + actionId;
        return configs.size();
    }
}
