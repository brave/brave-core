/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.messages;

import org.chromium.base.supplier.NonNullObservableSupplier;
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.ActivityTabProvider;
import org.chromium.chrome.browser.fullscreen.BrowserControlsManager;
import org.chromium.chrome.browser.layouts.LayoutStateProvider;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.components.browser_ui.bottomsheet.BottomSheetController;
import org.chromium.components.messages.ManagedMessageDispatcher;
import org.chromium.ui.modaldialog.ModalDialogManager;

/**
 * Brave override of {@link ChromeMessageQueueMediator}.
 *
 * <p>When the bottom address bar is active the top browser controls have zero height, causing
 * {@code BrowserControlsUtils.areTopControlsFullyVisible()} to always return {@code false}. This
 * makes the base {@code areBrowserControlsReady()} permanently return {@code false}, so the message
 * show-runnable is set but never triggered and banners (e.g. save-password) never appear.
 *
 * <p>The fix: if top controls have zero height there are no top controls to wait for, so we return
 * {@code true} immediately (guarded only by the destroyed-state check).
 */
@NullMarked
public class BraveMessageQueueMediator extends ChromeMessageQueueMediator {
    private final BrowserControlsManager mBrowserControlsManager;

    public BraveMessageQueueMediator(
            BrowserControlsManager browserControlsManager,
            MessageContainerCoordinator messageContainerCoordinator,
            ActivityTabProvider activityTabProvider,
            OneshotSupplier<LayoutStateProvider> layoutStateProviderOneShotSupplier,
            NonNullObservableSupplier<ModalDialogManager> modalDialogManagerSupplier,
            BottomSheetController bottomSheetController,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            ManagedMessageDispatcher messageDispatcher) {
        super(
                browserControlsManager,
                messageContainerCoordinator,
                activityTabProvider,
                layoutStateProviderOneShotSupplier,
                modalDialogManagerSupplier,
                bottomSheetController,
                activityLifecycleDispatcher,
                messageDispatcher);
        mBrowserControlsManager = browserControlsManager;
    }

    @Override
    boolean areBrowserControlsReady() {
        // When top controls have zero height (e.g. bottom address bar), there are
        // no top controls to wait for, so unblock message display immediately.
        if (mBrowserControlsManager.getTopControlsHeight() == 0) {
            return !isDestroyed();
        }
        return super.areBrowserControlsReady();
    }
}
