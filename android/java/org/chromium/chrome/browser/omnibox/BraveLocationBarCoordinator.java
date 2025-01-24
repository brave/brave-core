/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox;

import android.view.ActionMode;
import android.view.View;
import android.view.View.OnLongClickListener;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.merchant_viewer.MerchantTrustSignalsCoordinator;
import org.chromium.chrome.browser.omnibox.LocationBarMediator.OmniboxUma;
import org.chromium.chrome.browser.omnibox.LocationBarMediator.SaveOfflineButtonState;
import org.chromium.chrome.browser.omnibox.status.StatusCoordinator.PageInfoAction;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxSuggestionsDropdownScrollListener;
import org.chromium.chrome.browser.omnibox.suggestions.basic.BasicSuggestionProcessor.BookmarkState;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabWindowManager;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.menu_button.BraveMenuButtonCoordinator;
import org.chromium.components.omnibox.action.OmniboxActionDelegate;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.function.BooleanSupplier;

public class BraveLocationBarCoordinator extends LocationBarCoordinator {
    /**
     * {@link LocationBarCoordinator#mLocationBarMediator} is private so we add a private
     * `mLocationBarMediator` here so this code compiles and then remove it and make {@link
     * LocationBarCoordinator#mLocationBarMediator} protected via asm.
     */
    private LocationBarMediator mLocationBarMediator;

    /**
     * {@link LocationBarCoordinator#mUrlBar} is private so we add a private `mUrlBar` here so this
     * code compiles and then remove it and make {@link LocationBarCoordinator#mUrlBar} protected
     * via asm.
     */
    private View mUrlBar;

    private View mQRButton;

    public BraveLocationBarCoordinator(
            View locationBarLayout,
            View autocompleteAnchorView,
            ObservableSupplier<Profile> profileObservableSupplier,
            LocationBarDataProvider locationBarDataProvider,
            ActionMode.Callback actionModeCallback,
            WindowAndroid windowAndroid,
            @NonNull Supplier<Tab> activityTabSupplier,
            Supplier<ModalDialogManager> modalDialogManagerSupplier,
            Supplier<ShareDelegate> shareDelegateSupplier,
            IncognitoStateProvider incognitoStateProvider,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            OverrideUrlLoadingDelegate overrideUrlLoadingDelegate,
            BackKeyBehaviorDelegate backKeyBehavior,
            @NonNull PageInfoAction pageInfoAction,
            @NonNull Callback<Tab> bringTabToFrontCallback,
            @NonNull SaveOfflineButtonState saveOfflineButtonState,
            @NonNull OmniboxUma omniboxUma,
            @NonNull Supplier<TabWindowManager> tabWindowManagerSupplier,
            @NonNull BookmarkState bookmarkState,
            @NonNull BooleanSupplier isToolbarMicEnabledSupplier,
            @Nullable
                    Supplier<MerchantTrustSignalsCoordinator>
                            merchantTrustSignalsCoordinatorSupplier,
            @NonNull OmniboxActionDelegate omniboxActionDelegate,
            BrowserStateBrowserControlsVisibilityDelegate browserControlsVisibilityDelegate,
            @Nullable BackPressManager backPressManager,
            @Nullable
                    OmniboxSuggestionsDropdownScrollListener
                            omniboxSuggestionsDropdownScrollListener,
            @Nullable ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            LocationBarEmbedderUiOverrides uiOverrides,
            @Nullable View baseChromeLayout,
            Supplier<Integer> bottomWindowPaddingSupplier,
            @Nullable OnLongClickListener onLongClickListener,
            @Nullable BrowserControlsStateProvider browserControlsStateProvider,
            boolean isToolbarPositionCustomizationEnabled) {
        super(
                locationBarLayout,
                autocompleteAnchorView,
                profileObservableSupplier,
                locationBarDataProvider,
                actionModeCallback,
                windowAndroid,
                activityTabSupplier,
                modalDialogManagerSupplier,
                shareDelegateSupplier,
                incognitoStateProvider,
                activityLifecycleDispatcher,
                overrideUrlLoadingDelegate,
                backKeyBehavior,
                pageInfoAction,
                bringTabToFrontCallback,
                saveOfflineButtonState,
                omniboxUma,
                tabWindowManagerSupplier,
                bookmarkState,
                isToolbarMicEnabledSupplier,
                merchantTrustSignalsCoordinatorSupplier,
                omniboxActionDelegate,
                browserControlsVisibilityDelegate,
                backPressManager,
                omniboxSuggestionsDropdownScrollListener,
                tabModelSelectorSupplier,
                uiOverrides,
                baseChromeLayout,
                () ->
                        bottomWindowPaddingSupplier.get()
                                + (isBottomToolbarVisible() ? locationBarLayout.getHeight() : 0),
                onLongClickListener,
                browserControlsStateProvider,
                isToolbarPositionCustomizationEnabled);

        if (mUrlBar != null) {
            ((UrlBar) mUrlBar).setSelectAllOnFocus(true);
        }

        if (mLocationBarMediator instanceof BraveLocationBarMediator) {
            mQRButton = locationBarLayout.findViewById(R.id.qr_button);
            mQRButton.setOnClickListener(
                    ((BraveLocationBarMediator) mLocationBarMediator)::qrButtonClicked);
        }
    }

    @Override
    public void destroy() {
        super.destroy();
        if (mQRButton != null) {
            mQRButton.setOnClickListener(null);
            mQRButton = null;
        }
    }

    private static boolean isBottomToolbarVisible() {
        return BottomToolbarConfiguration.isBottomToolbarEnabled()
                && BraveMenuButtonCoordinator.isMenuFromBottom();
    }
}
