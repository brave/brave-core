/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_wallet;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.util.Arrays;
import java.util.List;

@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveWalletUtilsTest {
    @Test
    @SmallTest
    public void fromHexWeiTest() {
        assertEquals(Utils.fromHexWei("0x2B5E3AF16B0000000", 18), 50, 0.001);
        assertEquals(Utils.fromHexWei("0x4563918244F40000", 18), 5, 0.001);
        assertEquals(Utils.fromHexWei("0x6F05B59D3B20000", 18), 0.5, 0.001);
        assertEquals(Utils.fromHexWei("0xB1A2BC2EC50000", 18), 0.05, 0.001);
        assertEquals(Utils.fromHexWei("0x0", 18), 0, 0.001);
        assertEquals(Utils.fromHexWei("", 18), 0, 0.001);
    }

    @Test
    @SmallTest
    public void fromHexGWeiToGWEITest() {
        assertEquals(Utils.fromHexGWeiToGWEI("0xabcdf"), 703711, 0.001);
    }

    @Test
    @SmallTest
    public void toHexGWeiFromGWEITest() {
        assertEquals(Utils.toHexGWeiFromGWEI("703711"), "0xabcdf");
    }

    @Test
    @SmallTest
    public void toWeiHexTest() {
        assertEquals(Utils.toWeiHex("703711"), "0xabcdf");
    }

    @Test
    @SmallTest
    public void multiplyHexBNTest() {
        assertEquals(Utils.multiplyHexBN("0x123afff", "0xabcdf"), "0xc3c134b9321");
    }

    @Test
    @SmallTest
    public void concatHexBNTest() {
        assertEquals(Utils.concatHexBN("0x123afff", "0xabcdf"), "0x12e6cde");
    }

    @Test
    @SmallTest
    public void toHexWeiTest() {
        assertEquals(Utils.toHexWei("5.2", 18), "0x482a1c7300080000");
        assertEquals(Utils.toHexWei("5", 18), "0x4563918244f40000");
        assertEquals(Utils.toHexWei("0.5", 18), "0x6f05b59d3b20000");
        assertEquals(Utils.toHexWei("0.05", 18), "0xb1a2bc2ec50000");
        assertEquals(Utils.toHexWei("", 18), "0x0");
    }

    @Test
    @SmallTest
    public void fromWeiTest() {
        assertEquals(Utils.fromWei("50000000000000000000", 18), 50, 0.001);
        assertEquals(Utils.fromWei("5000000000000000000", 18), 5, 0.001);
        assertEquals(Utils.fromWei("500000000000000000", 18), 0.5, 0.001);
        assertEquals(Utils.fromWei("50000000000000000", 18), 0.05, 0.001);
        assertEquals(Utils.fromWei(null, 18), 0, 0.001);
        assertEquals(Utils.fromWei("", 18), 0, 0.001);
    }

    @Test
    @SmallTest
    public void toWei() {
        assertEquals(Utils.toWei("50", 18, false), "50000000000000000000");
        assertEquals(Utils.toWei("5", 18, false), "5000000000000000000");
        assertEquals(Utils.toWei("0.5", 18, false), "500000000000000000");
        assertEquals(Utils.toWei("0.05", 18, false), "50000000000000000");
        assertEquals(Utils.toWei("", 18, false), "");
        assertEquals(Utils.toWei("", 18, true), "");
    }

    @Test
    @SmallTest
    public void getRecoveryPhraseAsListTest() {
        List<String> recoveryPhrase =
                Utils.getRecoveryPhraseAsList("this is a fake recovery phrase");
        assertEquals(recoveryPhrase.size(), 6);
        assertEquals(recoveryPhrase.get(0), "this");
        assertEquals(recoveryPhrase.get(1), "is");
        assertEquals(recoveryPhrase.get(2), "a");
        assertEquals(recoveryPhrase.get(3), "fake");
        assertEquals(recoveryPhrase.get(4), "recovery");
        assertEquals(recoveryPhrase.get(5), "phrase");
    }

    @Test
    @SmallTest
    public void getRecoveryPhraseFromListTest() {
        List<String> list =
                Arrays.asList(new String[] {"this", "is", "a", "fake", "recovery", "phrase"});
        String recoveryPhrase = Utils.getRecoveryPhraseFromList(list);
        assertEquals(recoveryPhrase, "this is a fake recovery phrase");
    }

    @Test
    @SmallTest
    public void getBuyUrlForTestChainTest() {
        assertEquals(Utils.getBuyUrlForTestChain(BraveWalletConstants.RINKEBY_CHAIN_ID),
                "https://www.rinkeby.io/#stats");
        assertEquals(Utils.getBuyUrlForTestChain(BraveWalletConstants.ROPSTEN_CHAIN_ID),
                "https://faucet.ropsten.be/");
        assertEquals(Utils.getBuyUrlForTestChain(BraveWalletConstants.GOERLI_CHAIN_ID),
                "https://goerli-faucet.slock.it/");
        assertEquals(Utils.getBuyUrlForTestChain(BraveWalletConstants.KOVAN_CHAIN_ID),
                "https://github.com/kovan-testnet/faucet");
        assertEquals(Utils.getBuyUrlForTestChain("unknown"), "");
    }

    @Test
    @SmallTest
    public void getDecimalsDepNumberTest() {
        assertEquals(Utils.getDecimalsDepNumber(9), "1000000000");
        assertEquals(Utils.getDecimalsDepNumber(18), "1000000000000000000");
    }

    @Test
    @SmallTest
    public void hexStrToNumberArrayTest() {
        byte[] numberArray = Utils.hexStrToNumberArray("0x4f00abcd");
        assertEquals(numberArray.length, 4);
        assertEquals(numberArray[0], 79);
        assertEquals(numberArray[1], 0);
        assertEquals(numberArray[2], -85);
        assertEquals(numberArray[3], -51);
    }

    @Test
    @SmallTest
    public void numberArrayToHexStrTest() {
        byte[] numberArray = new byte[] {79, 0, -85, -51};
        assertEquals(Utils.numberArrayToHexStr(numberArray), "0x4f00abcd");
    }

    @Test
    @SmallTest
    public void stripAccountAddressTest() {
        assertEquals(Utils.stripAccountAddress("0xdef1c0ded9bec7f1a1670819833240f027b25eff"),
                "0xdef1***25eff");
    }

    @Test
    @SmallTest
    public void isJSONValidTest() {
        assertEquals(Utils.isJSONValid("{'name': 'brave'}"), true);
        assertEquals(Utils.isJSONValid("'name': 'brave'"), false);
    }

    @Test
    @SmallTest
    public void isSwapLiquidityErrorReasonTest() {
        assertEquals(
                Utils.isSwapLiquidityErrorReason(
                        "{code: 100, reason: 'Validation Failed', validationErrors:[{field: 'buyAmount', code: 1004, reason: 'INSUFFICIENT_ASSET_LIQUIDITY'}]}"),
                true);
        assertEquals(
                Utils.isSwapLiquidityErrorReason(
                        "{code: 100, reason: 'Validation Failed', validationErrors:[{field: 'buyAmount', code: 1004, reason: 'SOMETHING_ELSE'}]}"),
                false);
    }

    @Test
    @SmallTest
    public void getContractAddressTest() {
        assertEquals(Utils.getContractAddress(BraveWalletConstants.ROPSTEN_CHAIN_ID, "USDC",
                             "0xdef1c0ded9bec7f1a1670819833240f027b25eff"),
                "0x07865c6e87b9f70255377e024ace6630c1eaa37f");
        assertEquals(Utils.getContractAddress(BraveWalletConstants.ROPSTEN_CHAIN_ID, "DAI",
                             "0xdef1c0ded9bec7f1a1670819833240f027b25eff"),
                "0xad6d458402f60fd3bd25163575031acdce07538d");
        assertEquals(Utils.getContractAddress(BraveWalletConstants.ROPSTEN_CHAIN_ID, "BAT",
                             "0xdef1c0ded9bec7f1a1670819833240f027b25eff"),
                "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
        assertEquals(Utils.getContractAddress(BraveWalletConstants.RINKEBY_CHAIN_ID, "USDC",
                             "0xdef1c0ded9bec7f1a1670819833240f027b25eff"),
                "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
    }

    @Test
    @SmallTest
    public void getRopstenContractAddressTest() {
        assertEquals(Utils.getRopstenContractAddress("0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"),
                "0x07865c6e87b9f70255377e024ace6630c1eaa37f");
        assertEquals(Utils.getRopstenContractAddress("0x6b175474e89094c44da98b954eedeac495271d0f"),
                "0xad6d458402f60fd3bd25163575031acdce07538d");
        assertEquals(
                Utils.getRopstenContractAddress("0xdef1c0ded9bec7f1a1670819833240f027b25eff"), "");
    }
}
