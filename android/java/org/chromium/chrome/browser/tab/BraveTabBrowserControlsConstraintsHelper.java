/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tab;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

@NullMarked
public class BraveTabBrowserControlsConstraintsHelper {
    /** Returns the constraints delegate for a particular tab with destroy guard. */
    public static @Nullable MonotonicObservableSupplier<Integer> getObservableConstraints(Tab tab) {
        if (tab == null || tab.isDestroyed()) return null;
        return TabBrowserControlsConstraintsHelper.getObservableConstraints(tab);
    }
}
