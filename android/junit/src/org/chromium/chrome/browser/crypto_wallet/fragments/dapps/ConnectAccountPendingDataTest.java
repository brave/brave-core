/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.PermissionLifetimeOption;
import org.chromium.chrome.browser.crypto_wallet.fragments.dapps.ConnectAccountFragment.ConnectAccountPendingData;

/**
 * Tests for the pending-data hand-off from {@link ConnectAccountFragment} to {@link
 * org.chromium.chrome.browser.crypto_wallet.permission.BraveDappPermissionPromptDialog}. Pending
 * data arms an auto-confirm for the prompt the fragment is about to raise; only a prompt for the
 * origin and coin it was created for may consume it.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ConnectAccountPendingDataTest {
    private static final String ACCOUNT_ADDRESS = "0xb19d680e19bdc336b77bae9a6de5cd55b7f96ed8";
    private static final String SITE_A_ORIGIN = "http://127.0.0.1:8901/";
    private static final String SITE_B_ORIGIN = "http://127.0.0.1:8902/";

    @After
    public void tearDown() {
        // Leave the process-wide static empty for the next test.
        ConnectAccountFragment.getAndResetConnectAccountPendingData();
    }

    @Test
    public void pendingData_isConsumedExactlyOnce() {
        ConnectAccountFragment.setConnectAccountPendingData(
                ACCOUNT_ADDRESS, CoinType.ETH, SITE_A_ORIGIN);

        ConnectAccountPendingData capd =
                ConnectAccountFragment.getAndResetConnectAccountPendingData();
        assertNotNull(capd);
        assertEquals(ACCOUNT_ADDRESS, capd.accountAddress);
        assertEquals(SITE_A_ORIGIN, capd.originSpec);
        assertEquals(CoinType.ETH, capd.coinType);
        assertEquals(PermissionLifetimeOption.FOREVER, capd.permissionLifetimeOption);
        assertNull(ConnectAccountFragment.getAndResetConnectAccountPendingData());
    }

    @Test
    public void pendingData_foreignOrigin_isNotConsumed() {
        ConnectAccountFragment.setConnectAccountPendingData(
                ACCOUNT_ADDRESS, CoinType.ETH, SITE_A_ORIGIN);

        // A prompt raised for a different origin must not consume the data.
        assertNull(
                ConnectAccountFragment.getAndResetConnectAccountPendingDataIfMatches(
                        SITE_B_ORIGIN, CoinType.ETH));
        // The data is still armed for the origin it was created for.
        assertNotNull(
                ConnectAccountFragment.getAndResetConnectAccountPendingDataIfMatches(
                        SITE_A_ORIGIN, CoinType.ETH));
    }

    @Test
    public void pendingData_foreignCoin_isNotConsumed() {
        ConnectAccountFragment.setConnectAccountPendingData(
                ACCOUNT_ADDRESS, CoinType.ETH, SITE_A_ORIGIN);

        assertNull(
                ConnectAccountFragment.getAndResetConnectAccountPendingDataIfMatches(
                        SITE_A_ORIGIN, CoinType.SOL));
        assertNotNull(
                ConnectAccountFragment.getAndResetConnectAccountPendingDataIfMatches(
                        SITE_A_ORIGIN, CoinType.ETH));
    }

    @Test
    public void pendingData_solana_isConsumedForMatchingCoinOnly() {
        ConnectAccountFragment.setConnectAccountPendingData(
                ACCOUNT_ADDRESS, CoinType.SOL, SITE_A_ORIGIN);

        assertNull(
                ConnectAccountFragment.getAndResetConnectAccountPendingDataIfMatches(
                        SITE_A_ORIGIN, CoinType.ETH));
        assertNotNull(
                ConnectAccountFragment.getAndResetConnectAccountPendingDataIfMatches(
                        SITE_A_ORIGIN, CoinType.SOL));
    }

    @Test
    public void pendingData_isConsumedExactlyOnceIfMatching() {
        ConnectAccountFragment.setConnectAccountPendingData(
                ACCOUNT_ADDRESS, CoinType.ETH, SITE_A_ORIGIN);

        assertNotNull(
                ConnectAccountFragment.getAndResetConnectAccountPendingDataIfMatches(
                        SITE_A_ORIGIN, CoinType.ETH));
        assertNull(
                ConnectAccountFragment.getAndResetConnectAccountPendingDataIfMatches(
                        SITE_A_ORIGIN, CoinType.ETH));
    }

    @Test
    public void pendingData_staleFailure_doesNotDropNewerArm() {
        ConnectAccountPendingData staleArm =
                ConnectAccountFragment.setConnectAccountPendingData(
                        ACCOUNT_ADDRESS, CoinType.ETH, SITE_A_ORIGIN);
        ConnectAccountFragment.setConnectAccountPendingData(
                ACCOUNT_ADDRESS, CoinType.SOL, SITE_B_ORIGIN);

        ConnectAccountFragment.resetConnectAccountPendingDataIfSame(staleArm);

        // The newer arm is untouched by the stale failure callback.
        assertNotNull(
                ConnectAccountFragment.getAndResetConnectAccountPendingDataIfMatches(
                        SITE_B_ORIGIN, CoinType.SOL));
    }

    @Test
    public void pendingData_nullRequestingOrigin_isNotConsumed() {
        ConnectAccountFragment.setConnectAccountPendingData(
                ACCOUNT_ADDRESS, CoinType.ETH, SITE_A_ORIGIN);

        assertNull(
                ConnectAccountFragment.getAndResetConnectAccountPendingDataIfMatches(
                        null, CoinType.ETH));
    }

    @Test
    public void pendingData_withoutOrigin_neverMatches() {
        ConnectAccountFragment.setConnectAccountPendingData(ACCOUNT_ADDRESS, CoinType.ETH, null);

        assertNull(
                ConnectAccountFragment.getAndResetConnectAccountPendingDataIfMatches(
                        SITE_A_ORIGIN, CoinType.ETH));
    }
}
