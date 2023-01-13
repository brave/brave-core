/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.modules.ledger_hw;

import org.chromium.chrome.features.ledger_hw.LedgerHwModule;

/** Helpers for LedgerHw DFM installation. */
public class LedgerHwModuleProvider {
    public static boolean isModuleInstalled() {
        return LedgerHwModule.isInstalled();
    }

    public static void installModule(LedgerHwInstallListener listener) {
        LedgerHwModule.install(listener);
    }

    public static void ensureNativeLoaded() {
        LedgerHwModule.ensureNativeLoaded();
    }
}
