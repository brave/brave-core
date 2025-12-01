/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.appmenu;

import android.content.res.Resources;

import org.chromium.build.annotations.NullMarked;

/*
 * This is dummy super class for BraveAppMenu.
 * We need it to be able to call private methods of AppMenu.
 */
@NullMarked
class BraveAppMenuDummySuper extends AppMenu {
    BraveAppMenuDummySuper(AppMenuHandlerImpl handler, Resources res) {
        super(handler, res);
        assert false : "This class usage should be removed in the bytecode!";
    }

    public void runMenuItemEnterAnimations() {
        assert false : "This class usage should be removed in the bytecode!";
    }
}
