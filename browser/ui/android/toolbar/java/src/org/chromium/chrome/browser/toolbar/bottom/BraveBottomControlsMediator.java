/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.browser_controls.BottomControlsStacker;
import org.chromium.chrome.browser.browser_controls.BrowserStateBrowserControlsVisibilityDelegate;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.tab.TabObscuringHandler;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;

class BraveBottomControlsMediator extends BottomControlsMediator {
    // To delete in bytecode, members from parent class will be used instead.
    private PropertyModel mModel;
    private BottomControlsStacker mBottomControlsStacker;

    // Own members.
    private final ObservableSupplierImpl<Boolean> mTabGroupUiVisibleSupplier =
            new ObservableSupplierImpl<>();
    private final ObservableSupplierImpl<Boolean> mBottomToolbarVisibleSupplier =
            new ObservableSupplierImpl<>();
    private final int mBottomControlsHeightSingle;
    private final int mBottomControlsHeightDouble;

    BraveBottomControlsMediator(
            WindowAndroid windowAndroid,
            PropertyModel model,
            BottomControlsStacker controlsStacker,
            BrowserStateBrowserControlsVisibilityDelegate browserControlsVisibilityDelegate,
            FullscreenManager fullscreenManager,
            TabObscuringHandler tabObscuringHandler,
            int bottomControlsHeight,
            int bottomControlsShadowHeight,
            ObservableSupplier<Boolean> overlayPanelVisibilitySupplier,
            ObservableSupplier<EdgeToEdgeController> edgeToEdgeControllerSupplier,
            Supplier<Boolean> readAloudRestoringSupplier) {
        super(
                windowAndroid,
                model,
                controlsStacker,
                browserControlsVisibilityDelegate,
                fullscreenManager,
                tabObscuringHandler,
                bottomControlsHeight,
                bottomControlsShadowHeight,
                overlayPanelVisibilitySupplier,
                edgeToEdgeControllerSupplier,
                readAloudRestoringSupplier);

        mTabGroupUiVisibleSupplier.set(false);
        mBottomToolbarVisibleSupplier.set(false);
        mBottomControlsHeightSingle = bottomControlsHeight;
        mBottomControlsHeightDouble = bottomControlsHeight * 2;
    }

    @Override
    public void setBottomControlsVisible(boolean visible) {
        mTabGroupUiVisibleSupplier.set(visible);
        // We should keep it visible if bottom toolbar is visible.
        super.setBottomControlsVisible(mBottomToolbarVisibleSupplier.get() || visible);
        updateYOffset();
    }

    public void setBottomToolbarVisible(boolean visible) {
        mBottomToolbarVisibleSupplier.set(visible);
        // We should keep it visible if tag group UI is visible.
        super.setBottomControlsVisible(mTabGroupUiVisibleSupplier.get() || visible);
        updateYOffset();
    }

    public ObservableSupplierImpl<Boolean> getBottomToolbarVisibleSupplier() {
        return mBottomToolbarVisibleSupplier;
    }

    public ObservableSupplierImpl<Boolean> getTabGroupUiVisibleSupplier() {
        return mTabGroupUiVisibleSupplier;
    }

    private void updateYOffset() {
        // This indicates that both controls are visible, but bottom toolbar has already been
        // scrolled down, so we move scroll further for tab groups control.
        if (bothBottomControlsVisible()
                && mBottomControlsStacker.getBrowserControls().getBottomControlOffset()
                        == mBottomControlsHeightSingle) {
            mModel.set(BottomControlsProperties.Y_OFFSET, mBottomControlsHeightDouble);
        }
    }

    @Override
    public int getHeight() {
        if (bothBottomControlsVisible()) {
            // Factor in the height of the Brave navigation bottom controls when they are visible.
            return super.getHeight() + mBottomControlsHeightSingle;
        }

        return super.getHeight();
    }

    private boolean bothBottomControlsVisible() {
        return mTabGroupUiVisibleSupplier.get() && mBottomToolbarVisibleSupplier.get();
    }
}
