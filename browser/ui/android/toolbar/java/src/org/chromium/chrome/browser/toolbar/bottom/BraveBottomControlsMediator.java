/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.bottom;

import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.chrome.browser.browser_controls.BrowserControlsSizer;
import org.chromium.chrome.browser.fullscreen.FullscreenManager;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.modelutil.PropertyModel;

class BraveBottomControlsMediator extends BottomControlsMediator {
    // To delete in bytecode, members from parent class will be used instead.
    private int mBottomControlsHeight;
    private PropertyModel mModel;
    private BrowserControlsSizer mBrowserControlsSizer;

    // Own members.
    private ObservableSupplierImpl<Boolean> mTabGroupUiVisibleSupplier =
            new ObservableSupplierImpl<>();
    private ObservableSupplierImpl<Boolean> mBottomToolbarVisibleSupplier =
            new ObservableSupplierImpl<>();
    private int mBottomControlsHeightSingle;
    private int mBottomControlsHeightDouble;

    BraveBottomControlsMediator(WindowAndroid windowAndroid, PropertyModel model,
            BrowserControlsSizer controlsSizer, FullscreenManager fullscreenManager,
            int bottomControlsHeight, ObservableSupplier<Boolean> overlayPanelVisibilitySupplier) {
        super(windowAndroid, model, controlsSizer, fullscreenManager, bottomControlsHeight,
                overlayPanelVisibilitySupplier);

        mTabGroupUiVisibleSupplier.set(false);
        mBottomToolbarVisibleSupplier.set(false);
        mBottomControlsHeightSingle = bottomControlsHeight;
        mBottomControlsHeightDouble = bottomControlsHeight * 2;
    }

    @Override
    public void setBottomControlsVisible(boolean visible) {
        mTabGroupUiVisibleSupplier.set(visible);
        updateBottomControlsHeight();
        // We should keep it visible if bottom toolbar is visible.
        super.setBottomControlsVisible(mBottomToolbarVisibleSupplier.get() || visible);
    }

    public void setBottomToolbarVisible(boolean visible) {
        mBottomToolbarVisibleSupplier.set(visible);
        updateBottomControlsHeight();
        // We should keep it visible if tag group UI is visible.
        super.setBottomControlsVisible(mTabGroupUiVisibleSupplier.get() || visible);
    }

    public ObservableSupplierImpl<Boolean> getBottomToolbarVisibleSupplier() {
        return mBottomToolbarVisibleSupplier;
    }

    public ObservableSupplierImpl<Boolean> getTabGroupUiVisibleSupplier() {
        return mTabGroupUiVisibleSupplier;
    }

    private void updateBottomControlsHeight() {
        if (mBottomToolbarVisibleSupplier.get() && mTabGroupUiVisibleSupplier.get()) {
            // Double the height if both bottom controls are visible
            mBottomControlsHeight = mBottomControlsHeightDouble;
        } else {
            mBottomControlsHeight = mBottomControlsHeightSingle;
        }
    }

    public void updateCompositedViewVisibility() {
        final boolean isCompositedViewVisible = isCompositedViewVisible();
        mModel.set(BottomControlsProperties.COMPOSITED_VIEW_VISIBLE, isCompositedViewVisible);
        mBrowserControlsSizer.setBottomControlsHeight(isCompositedViewVisible
                        ? mBottomControlsHeight
                        : (mBottomControlsHeight - mBottomControlsHeightSingle),
                mBrowserControlsSizer.getBottomControlsMinHeight());
    }
}
