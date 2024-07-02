/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.browser_controls.BottomControlsStacker;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.chrome.browser.tab.TabObscuringHandler;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;

class BraveBottomControlsMediator extends BottomControlsMediator {
    // To delete in bytecode, members from parent class will be used instead.
    private int mBottomControlsHeight;
    private PropertyModel mModel;
    private BottomControlsStacker mBottomControlsStacker;

    // Own members.
    private ObservableSupplierImpl<Boolean> mTabGroupUiVisibleSupplier =
            new ObservableSupplierImpl<>();
    private ObservableSupplierImpl<Boolean> mBottomToolbarVisibleSupplier =
            new ObservableSupplierImpl<>();
    private int mBottomControlsHeightSingle;
    private int mBottomControlsHeightDouble;

    BraveBottomControlsMediator(
            WindowAndroid windowAndroid,
            PropertyModel model,
            BottomControlsStacker controlsStacker,
            FullscreenManager fullscreenManager,
            TabObscuringHandler tabObscuringHandler,
            int bottomControlsHeight,
            ObservableSupplier<Boolean> overlayPanelVisibilitySupplier,
            ObservableSupplier<EdgeToEdgeController> edgeToEdgeControllerSupplier,
            Supplier<Boolean> readAloudRestoringSupplier) {
        super(
                windowAndroid,
                model,
                controlsStacker,
                fullscreenManager,
                tabObscuringHandler,
                bottomControlsHeight,
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
        updateBottomControlsHeight(mBottomToolbarVisibleSupplier.get() && visible);
        // We should keep it visible if bottom toolbar is visible.
        super.setBottomControlsVisible(mBottomToolbarVisibleSupplier.get() || visible);
        mTabGroupUiVisibleSupplier.set(visible);
        updateYOffset();
    }

    public void setBottomToolbarVisible(boolean visible) {
        updateBottomControlsHeight(mTabGroupUiVisibleSupplier.get() && visible);
        // We should keep it visible if tag group UI is visible.
        super.setBottomControlsVisible(mTabGroupUiVisibleSupplier.get() || visible);
        mBottomToolbarVisibleSupplier.set(visible);
        updateYOffset();
    }

    public ObservableSupplierImpl<Boolean> getBottomToolbarVisibleSupplier() {
        return mBottomToolbarVisibleSupplier;
    }

    public ObservableSupplierImpl<Boolean> getTabGroupUiVisibleSupplier() {
        return mTabGroupUiVisibleSupplier;
    }

    private void updateBottomControlsHeight(boolean bothBottomControlsVisible) {
        // Double the height if both bottom controls are visible
        mBottomControlsHeight = bothBottomControlsVisible ? mBottomControlsHeightDouble
                                                          : mBottomControlsHeightSingle;
    }

    private void updateYOffset() {
        // This indicates that both controls are visible, but bottom toolbar has already been
        // scrolled down, so we move scroll further for tab groups control.
        if (mBottomControlsHeight == mBottomControlsHeightDouble
                && mBottomControlsStacker.getBrowserControls().getBottomControlOffset()
                        == mBottomControlsHeightSingle) {
            mModel.set(BottomControlsProperties.Y_OFFSET, mBottomControlsHeightDouble);
        }
    }
}
