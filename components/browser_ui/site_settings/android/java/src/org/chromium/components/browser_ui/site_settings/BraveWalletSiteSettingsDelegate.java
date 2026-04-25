/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.browser_ui.site_settings;

import org.chromium.build.annotations.NullMarked;

/**
 * Interface for wallet-related site settings delegate methods. Implemented by
 * BraveSiteSettingsDelegate to provide wallet policy information to components-level code without
 * creating layering violations.
 */
@NullMarked
public interface BraveWalletSiteSettingsDelegate {
    /** Returns true if Brave Wallet is disabled by enterprise policy. */
    boolean isWalletDisabledByPolicy();
}
