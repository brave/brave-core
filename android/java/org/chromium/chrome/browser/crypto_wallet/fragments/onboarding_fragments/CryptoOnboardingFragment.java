/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.onboarding_fragments;

import androidx.fragment.app.Fragment;

import org.chromium.chrome.browser.crypto_wallet.listeners.OnNextPage;

public class CryptoOnboardingFragment extends Fragment {
    protected OnNextPage onNextPage;

    public void setOnNextPageListener(OnNextPage onNextPage) {
        this.onNextPage = onNextPage;
    }
}
