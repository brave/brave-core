/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.appmenu;

import android.content.Context;
import android.graphics.Rect;
import android.view.View;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.browser_controls.BrowserControlsStateProvider;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.ui.base.WindowAndroid;

import java.util.function.Supplier;

/**
 * Brave extension for {@link AppMenuHandlerImpl}. See {@code BraveAppMenuHandlerImplClassAdapter}
 * for bytecode manipulation logic. See {@code BytecodeTest} for bytecode related tests.
 */
@NullMarked
class BraveAppMenuHandlerImpl extends AppMenuHandlerImpl {
    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private @Nullable AppMenu mAppMenu;

    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private @Nullable AppMenuDragHelper mAppMenuDragHelper;

    public BraveAppMenuHandlerImpl(
            Context context,
            AppMenuPropertiesDelegate delegate,
            AppMenuDelegate appMenuDelegate,
            View decorView,
            ActivityLifecycleDispatcher activityLifecycleDispatcher,
            View hardwareButtonAnchorView,
            Supplier<Rect> appRect,
            WindowAndroid windowAndroid,
            BrowserControlsStateProvider browserControlsStateProvider) {
        super(
                context,
                delegate,
                appMenuDelegate,
                decorView,
                activityLifecycleDispatcher,
                hardwareButtonAnchorView,
                appRect,
                windowAndroid,
                browserControlsStateProvider);
    }

    @Override
    boolean showAppMenu(@Nullable View anchorView, boolean startDragging) {
        final boolean show = super.showAppMenu(anchorView, startDragging);
        if (show) {
            assert mAppMenuDragHelper != null;
            mAppMenuDragHelper.finishDragging();
            assert mAppMenu != null;
            if (mAppMenu.getListView() != null) {
               mAppMenu.getListView().scrollTo(0, 0);
            }
        }
        return show;
    }
}
