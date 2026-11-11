/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertTrue;

import android.app.Activity;

import androidx.preference.Preference;
import androidx.test.filters.SmallTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CriteriaHelper;
import org.chromium.base.test.util.DoNotBatch;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo.system.MojoResult;

/** Tests Wallet settings when no browser activity owns a WalletModel. */
@RunWith(ChromeJUnit4ClassRunner.class)
@DoNotBatch(reason = "Requires Wallet settings to launch without a browser activity.")
public class BraveWalletPreferencesTest {
    private static final String DEFAULT_ETHEREUM_WALLET = "default_ethereum_wallet";
    private static final String DEFAULT_SOLANA_WALLET = "default_solana_wallet";
    private static final String NFT_DISCOVERY = "nft_auto_discovery_switch";
    private static final String NFT_DESCRIPTION = "nft_auto_discovery_learn_more";

    private static final class ExpectedPreferences {
        final int mEthereumIndex;
        final int mSolanaIndex;
        final boolean mNftEnabled;

        ExpectedPreferences(int ethereumIndex, int solanaIndex, boolean nftEnabled) {
            mEthereumIndex = ethereumIndex;
            mSolanaIndex = solanaIndex;
            mNftEnabled = nftEnabled;
        }
    }

    @Rule
    public final SettingsActivityTestRule<BraveWalletPreferences> mSettingsActivityTestRule =
            new SettingsActivityTestRule<>(BraveWalletPreferences.class);

    @Test
    @SmallTest
    public void testPreferencesWithoutBrowserActivitySurviveRecreation() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> ChromeBrowserInitializer.getInstance().handleSynchronousStartup());
        mSettingsActivityTestRule.startSettingsActivity();
        BraveWalletPreferences fragment = mSettingsActivityTestRule.getFragment();
        assertPreferencesInitialized(fragment);

        ExpectedPreferences expected =
                ThreadUtils.runOnUiThreadBlocking(
                        () -> {
                            BraveDialogPreference ethereum =
                                    fragment.findPreference(DEFAULT_ETHEREUM_WALLET);
                            BraveDialogPreference solana =
                                    fragment.findPreference(DEFAULT_SOLANA_WALLET);
                            ChromeSwitchPreference nftDiscovery =
                                    fragment.findPreference(NFT_DISCOVERY);
                            assertNotNull(ethereum);
                            assertNotNull(solana);
                            assertNotNull(nftDiscovery);
                            int ethereumIndex = 1 - ethereum.getCheckedIndex();
                            int solanaIndex = 1 - solana.getCheckedIndex();
                            boolean nftEnabled = !nftDiscovery.isChecked();
                            assertTrue(ethereum.callChangeListener(ethereumIndex));
                            assertTrue(solana.callChangeListener(solanaIndex));
                            nftDiscovery.performClick();
                            assertEquals(nftEnabled, nftDiscovery.isChecked());
                            return new ExpectedPreferences(ethereumIndex, solanaIndex, nftEnabled);
                        });

        mSettingsActivityTestRule.recreateActivity();
        BraveWalletPreferences restoredFragment = mSettingsActivityTestRule.getFragment();
        assertNotSame(fragment, restoredFragment);
        assertPreferencesInitialized(restoredFragment);
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    BraveDialogPreference ethereum =
                            restoredFragment.findPreference(DEFAULT_ETHEREUM_WALLET);
                    BraveDialogPreference solana =
                            restoredFragment.findPreference(DEFAULT_SOLANA_WALLET);
                    ChromeSwitchPreference nftDiscovery =
                            restoredFragment.findPreference(NFT_DISCOVERY);
                    assertNotNull(ethereum);
                    assertNotNull(solana);
                    assertNotNull(nftDiscovery);
                    assertEquals(expected.mEthereumIndex, ethereum.getCheckedIndex());
                    assertEquals(expected.mSolanaIndex, solana.getCheckedIndex());
                    assertEquals(expected.mNftEnabled, nftDiscovery.isChecked());
                });
    }

    @Test
    @SmallTest
    public void testConnectionErrorWhileActivityFinishesDoesNotUpdatePreferences() {
        ThreadUtils.runOnUiThreadBlocking(
                () -> ChromeBrowserInitializer.getInstance().handleSynchronousStartup());
        mSettingsActivityTestRule.startSettingsActivity();
        BraveWalletPreferences fragment = mSettingsActivityTestRule.getFragment();
        assertPreferencesInitialized(fragment);

        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    Activity activity = mSettingsActivityTestRule.getActivity();
                    activity.finish();
                    assertTrue(activity.isFinishing());
                    assertTrue(fragment.isAdded());
                    fragment.onConnectionError(new MojoException(MojoResult.FAILED_PRECONDITION));
                    assertTrue(fragment.findPreference(DEFAULT_ETHEREUM_WALLET).isEnabled());
                    assertTrue(fragment.findPreference(DEFAULT_SOLANA_WALLET).isEnabled());
                    assertTrue(fragment.findPreference(NFT_DISCOVERY).isEnabled());
                });
    }

    private void assertPreferencesInitialized(BraveWalletPreferences fragment) {
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    for (Activity activity : ApplicationStatus.getRunningActivities()) {
                        assertFalse(
                                "Test must not have a browser WalletModel.",
                                activity instanceof BraveActivity);
                    }
                    Preference description = fragment.findPreference(NFT_DESCRIPTION);
                    assertNotNull(description);
                    assertNotNull(description.getTitle());
                    assertTrue(description.getTitle().length() > 0);
                });
        CriteriaHelper.pollUiThread(
                () -> {
                    Preference ethereum = fragment.findPreference(DEFAULT_ETHEREUM_WALLET);
                    Preference solana = fragment.findPreference(DEFAULT_SOLANA_WALLET);
                    Preference nftDiscovery = fragment.findPreference(NFT_DISCOVERY);
                    assertNotNull(ethereum);
                    assertNotNull(solana);
                    assertNotNull(nftDiscovery);
                    assertTrue(
                            "Ethereum wallet preference never initialized.", ethereum.isEnabled());
                    assertNotNull(ethereum.getSummary());
                    assertTrue("Solana wallet preference never initialized.", solana.isEnabled());
                    assertNotNull(solana.getSummary());
                    assertTrue(
                            "NFT discovery preference never initialized.",
                            nftDiscovery.isEnabled());
                });
    }
}
