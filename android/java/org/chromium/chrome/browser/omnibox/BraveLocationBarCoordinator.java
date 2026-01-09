/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox;

import static org.chromium.build.NullUtil.assertNonNull;

import android.graphics.Bitmap;
import android.view.ActionMode;
import android.view.View;
import android.view.View.OnLongClickListener;

import org.chromium.base.Callback;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.back_press.BackPressManager;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.merchant_viewer.MerchantTrustSignalsCoordinator;
import org.chromium.chrome.browser.multiwindow.MultiInstanceManager;
import org.chromium.chrome.browser.omnibox.LocationBarMediator.OmniboxUma;
import org.chromium.chrome.browser.omnibox.status.StatusCoordinator.PageInfoAction;
import org.chromium.chrome.browser.omnibox.suggestions.OmniboxSuggestionsDropdownScrollListener;
import org.chromium.chrome.browser.omnibox.suggestions.basic.BasicSuggestionProcessor.BookmarkState;
import org.chromium.chrome.browser.page_info.ChromePageInfoHighlight;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.share.ShareDelegate;
import org.chromium.chrome.browser.shields.BraveUnifiedPanelHandler;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.menu_button.BraveMenuButtonCoordinator;
import org.chromium.chrome.browser.ui.messages.snackbar.SnackbarManager;
import org.chromium.components.browser_ui.accessibility.PageZoomManager;
import org.chromium.components.omnibox.action.OmniboxActionDelegate;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modaldialog.ModalDialogManager;

import java.util.function.BooleanSupplier;
import java.util.function.Function;
import java.util.function.Supplier;

@NullMarked
public class BraveLocationBarCoordinator extends LocationBarCoordinator {
    /**
     * Wrapper for PageInfoAction that delegates to the unified panel handler. This allows us to
     * initialize the handler after super() is called.
     */
    private static class BravePageInfoActionWrapper implements PageInfoAction {
        private @Nullable BraveUnifiedPanelHandler mHandler;
        private final View mLocationBarLayout;

        public BravePageInfoActionWrapper(View locationBarLayout) {
            mLocationBarLayout = locationBarLayout;
        }

        public void initialize(BraveUnifiedPanelHandler handler) {
            mHandler = handler;
        }

        @Override
        public void show(Tab tab, ChromePageInfoHighlight pageInfoHighlight) {
            if (mHandler == null || mLocationBarLayout == null || tab == null) {
                return;
            }

            // Get the StatusView to use as anchor
            View anchorView = null;
            if (mLocationBarLayout instanceof BraveLocationBarLayout) {
                anchorView = ((BraveLocationBarLayout) mLocationBarLayout).getStatusView();
            }

            if (anchorView != null) {
                // Get the shields handler from the toolbar layout
                org.chromium.chrome.browser.shields.BraveShieldsHandler shieldsHandler = null;
                android.view.ViewParent parent = mLocationBarLayout.getParent();
                while (parent != null) {
                    if (parent
                            instanceof
                            org.chromium.chrome.browser.toolbar.top.BraveToolbarLayoutImpl) {
                        shieldsHandler =
                                ((org.chromium.chrome.browser.toolbar.top.BraveToolbarLayoutImpl)
                                                parent)
                                        .getBraveShieldsHandler();
                        break;
                    }
                    parent = parent.getParent();
                }

                if (shieldsHandler != null) {
                    mHandler.show(anchorView, tab, shieldsHandler);
                }
            }
        }
    }

    /**
     * {@link LocationBarCoordinator#mLocationBarMediator} is private so we add a private
     * `mLocationBarMediator` here so this code compiles and then remove it and make {@link
     * LocationBarCoordinator#mLocationBarMediator} protected via asm.
     */
    private @Nullable LocationBarMediator mLocationBarMediator;

    /**
     * {@link LocationBarCoordinator#mUrlBar} is private so we add a private `mUrlBar` here so this
     * code compiles and then remove it and make {@link LocationBarCoordinator#mUrlBar} protected
     * via asm.
     */
    private @Nullable View mUrlBar;

    private @Nullable View mQRButton;
    private final BraveUnifiedPanelHandler mUnifiedPanelHandler;

    public BraveLocationBarCoordinator(
            View locationBarLayout,
            View autocompleteAnchorView,
            ObservableSupplier<Profile> profileObservableSupplier,
            LocationBarDataProvider locationBarDataProvider,
            ActionMode.@Nullable Callback actionModeCallback,
            WindowAndroid windowAndroid,
            Supplier<@Nullable Tab> activityTabSupplier,
            Supplier<@Nullable ModalDialogManager> modalDialogManagerSupplier,
            @Nullable Supplier<ShareDelegate> shareDelegateSupplier,
            @Nullable IncognitoStateProvider incognitoStateProvider,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            OverrideUrlLoadingDelegate overrideUrlLoadingDelegate,
            BackKeyBehaviorDelegate backKeyBehavior,
            PageInfoAction pageInfoAction,
            Callback<String> bringTabGroupToFrontCallback,
            OmniboxUma omniboxUma,
            BookmarkState bookmarkState,
            BooleanSupplier isToolbarMicEnabledSupplier,
            @Nullable Supplier<MerchantTrustSignalsCoordinator>
                    merchantTrustSignalsCoordinatorSupplier,
            OmniboxActionDelegate omniboxActionDelegate,
            @Nullable BrowserStateBrowserControlsVisibilityDelegate
                    browserControlsVisibilityDelegate,
            @Nullable BackPressManager backPressManager,
            @Nullable OmniboxSuggestionsDropdownScrollListener
                    omniboxSuggestionsDropdownScrollListener,
            ObservableSupplier<TabModelSelector> tabModelSelectorSupplier,
            LocationBarEmbedder locationBarEmbedder,
            LocationBarEmbedderUiOverrides uiOverrides,
            @Nullable View baseChromeLayout,
            Supplier<Integer> bottomWindowPaddingSupplier,
            @Nullable OnLongClickListener onLongClickListener,
            @Nullable BrowserControlsStateProvider browserControlsStateProvider,
            boolean isToolbarPositionCustomizationEnabled,
            @Nullable PageZoomManager pageZoomManager,
            Function<Tab, @Nullable Bitmap> tabFaviconFunction,
            @Nullable MultiInstanceManager multiInstanceManager,
            SnackbarManager snackbarManager,
            View bottomContainerView) {
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
                createPageInfoActionWrapper(locationBarLayout), // Use our custom action
                // bringTabToFrontCallback,
                bringTabGroupToFrontCallback,
                omniboxUma,
                bookmarkState,
                isToolbarMicEnabledSupplier,
                merchantTrustSignalsCoordinatorSupplier,
                omniboxActionDelegate,
                browserControlsVisibilityDelegate,
                backPressManager,
                omniboxSuggestionsDropdownScrollListener,
                tabModelSelectorSupplier,
                locationBarEmbedder,
                uiOverrides,
                baseChromeLayout,
                () ->
                        bottomWindowPaddingSupplier.get()
                                + (isBraveBottomControlsVisible()
                                        ? locationBarLayout.getHeight()
                                        : 0),
                onLongClickListener,
                browserControlsStateProvider,
                isToolbarPositionCustomizationEnabled,
                pageZoomManager,
                tabFaviconFunction,
                multiInstanceManager,
                snackbarManager,
                bottomContainerView);

        if (mUrlBar != null) {
            ((UrlBar) mUrlBar).setSelectAllOnFocus(true);
        }

        assertNonNull(mLocationBarMediator);
        if (mLocationBarMediator instanceof BraveLocationBarMediator) {
            mQRButton = locationBarLayout.findViewById(R.id.qr_button);
            mQRButton.setOnClickListener(
                    ((BraveLocationBarMediator) mLocationBarMediator)::qrButtonClicked);
        }

        // Now initialize after super() has been called
        // Retrieve the wrapper from thread local and initialize it
        BravePageInfoActionWrapper wrapper = sPageInfoActionWrapper.get();

        // Initialize unified panel handler
        BraveUnifiedPanelHandler handler =
                new BraveUnifiedPanelHandler(locationBarLayout.getContext());
        mUnifiedPanelHandler = handler;

        if (wrapper != null) {
            wrapper.initialize(handler);
            sPageInfoActionWrapper.remove(); // Clean up
        }
    }

    private static final ThreadLocal<BravePageInfoActionWrapper> sPageInfoActionWrapper =
            new ThreadLocal<>();

    private static PageInfoAction createPageInfoActionWrapper(View locationBarLayout) {
        BravePageInfoActionWrapper wrapper = new BravePageInfoActionWrapper(locationBarLayout);
        // Store it in a thread local to retrieve after super() call
        sPageInfoActionWrapper.set(wrapper);
        return wrapper;
    }

    @Override
    public void destroy() {
        super.destroy();
        if (mQRButton != null) {
            mQRButton.setOnClickListener(null);
            mQRButton = null;
        }
        if (mUnifiedPanelHandler != null) {
            mUnifiedPanelHandler.hide();
        }
    }

    private static boolean isBraveBottomControlsVisible() {
        return BottomToolbarConfiguration.isBraveBottomControlsEnabled()
                && BraveMenuButtonCoordinator.isMenuFromBottom();
    }
}
