/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.listeners;

/**
 * Interface implemented by {@link org.chromium.chrome.browser.app.BraveActivity} in charge of
 * defining the navigation behavior for onboarding, biometric prompt, next page, and navigation
 * icons.
 */
public interface OnNextPage {
    void incrementPages(int pages);

    void showWallet(final boolean forceNewTab);

    void gotoCreationPage();

    void gotoRestorePage(final boolean isOnboarding);

    void showCloseButton(final boolean show);

    void showBackButton(final boolean show);
}
