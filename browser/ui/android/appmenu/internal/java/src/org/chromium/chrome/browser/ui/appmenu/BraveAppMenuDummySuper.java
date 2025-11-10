/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.appmenu;

import android.content.Context;
import android.content.res.Resources;
import android.view.View;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.ui.hierarchicalmenu.HierarchicalMenuController;

/*
 * This is dummy super class for BraveAppMenu.
 * We need it to be able to call private methods of AppMenu.
 */
@NullMarked
class BraveAppMenuDummySuper extends AppMenu {
    BraveAppMenuDummySuper(
            AppMenuHandlerImpl handler,
            Resources res,
            HierarchicalMenuController hierarchicalMenuController) {
        super(handler, res, hierarchicalMenuController);
        assert false : "This class usage should be removed in the bytecode!";
    }

    @Nullable
    public View createAppMenuContentView(Context context, boolean addTopPaddingBeforeFirstRow) {
        assert false : "This class usage should be removed in the bytecode!";
        return null;
    }

    public void runMenuItemEnterAnimations() {
        assert false : "This class usage should be removed in the bytecode!";
    }
}
