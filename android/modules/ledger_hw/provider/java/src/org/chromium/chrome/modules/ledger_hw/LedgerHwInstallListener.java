/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.modules.ledger_hw;

import org.chromium.components.module_installer.engine.InstallListener;

public class LedgerHwInstallListener implements InstallListener {
    public interface LedgerHwInstallListenerDelegate {
        default void onComplete(boolean success) {}
    }

    private LedgerHwInstallListenerDelegate mDelegate;

    @Override
    public void onComplete(boolean success) {
        if (mDelegate == null) {
            return;
        }

        mDelegate.onComplete(success);
    }

    public LedgerHwInstallListener(LedgerHwInstallListenerDelegate delegate) {
        mDelegate = delegate;
    }
}
