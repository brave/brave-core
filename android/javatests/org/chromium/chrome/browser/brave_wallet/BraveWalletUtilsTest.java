/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_wallet;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Batch;
import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.GasEstimation1559;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.RoutePriority;
import org.chromium.brave_wallet.mojom.SwapQuoteParams;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.Validations;
import org.chromium.url.mojom.Url;

import java.util.Arrays;
import java.util.List;
import java.util.Locale;

@Batch(Batch.PER_CLASS)
@RunWith(BaseJUnit4ClassRunner.class)
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
    public void toHexWeiEnTest() {
        Locale defaultLocal = Locale.getDefault();
        Locale.setDefault(Locale.US);
        assertEquals(Utils.toHexWei("5.2", 18), "0x482a1c7300080000");
        assertEquals(Utils.toHexWei("5", 18), "0x4563918244f40000");
        assertEquals(Utils.toHexWei("0.5", 18), "0x6f05b59d3b20000");
        assertEquals(Utils.toHexWei("0.05", 18), "0xb1a2bc2ec50000");
        assertEquals(Utils.toHexWei("0.01234567890123456789012", 18), "0x2bdc545d6b4b87");
        assertEquals(Utils.toHexWei("", 18), "0x0");
        Locale.setDefault(defaultLocal);
    }

    @Test
    @SmallTest
    public void toHexWeiFrTest() {
        Locale defaultLocal = Locale.getDefault();
        Locale.setDefault(Locale.FRANCE);
        assertEquals(Utils.toHexWei("5,2", 18), "0x482a1c7300080000");
        assertEquals(Utils.toHexWei("5", 18), "0x4563918244f40000");
        assertEquals(Utils.toHexWei("0,5", 18), "0x6f05b59d3b20000");
        assertEquals(Utils.toHexWei("0,05", 18), "0xb1a2bc2ec50000");
        assertEquals(Utils.toHexWei("0,01234567890123456789012", 18), "0x2bdc545d6b4b87");
        assertEquals(Utils.toHexWei("", 18), "0x0");
        Locale.setDefault(defaultLocal);
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
                "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
    }

    @Test
    @SmallTest
    public void isJSONValidTest() {
        assertEquals(Utils.isJSONValid("{'name': 'brave'}"), true);
        assertEquals(Utils.isJSONValid("'name': 'brave'"), false);
    }

    @Test
    @SmallTest
    public void getContractAddressTest() {
        assertEquals(Utils.getContractAddress(BraveWalletConstants.GOERLI_CHAIN_ID, "USDC",
                             "0xdef1c0ded9bec7f1a1670819833240f027b25eff"),
                "0x2f3a40a3db8a7e3d09b0adfefbce4f6f81927557");
        assertEquals(Utils.getContractAddress(BraveWalletConstants.GOERLI_CHAIN_ID, "DAI",
                             "0xdef1c0ded9bec7f1a1670819833240f027b25eff"),
                "0x73967c6a0904aa032c103b4104747e88c566b1a2");
        assertEquals(Utils.getContractAddress(BraveWalletConstants.GOERLI_CHAIN_ID, "BAT",
                             "0xdef1c0ded9bec7f1a1670819833240f027b25eff"),
                "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
        assertEquals(Utils.getContractAddress(BraveWalletConstants.SEPOLIA_CHAIN_ID, "USDC",
                             "0xdef1c0ded9bec7f1a1670819833240f027b25eff"),
                "0xdef1c0ded9bec7f1a1670819833240f027b25eff");
    }

    @Test
    @SmallTest
    public void getGoerliContractAddressTest() {
        assertEquals(Utils.getGoerliContractAddress("0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48"),
                "0x2f3a40a3db8a7e3d09b0adfefbce4f6f81927557");
        assertEquals(Utils.getGoerliContractAddress("0x6b175474e89094c44da98b954eedeac495271d0f"),
                "0x73967c6a0904aa032c103b4104747e88c566b1a2");
        assertEquals(
                Utils.getGoerliContractAddress("0xdef1c0ded9bec7f1a1670819833240f027b25eff"), "");
    }

    private static String getStackTrace(Exception ex) {
        String stack = "";
        StackTraceElement[] st = ex.getStackTrace();
        stack += ex.getClass().getName() + ": " + ex.getMessage() + "\n";
        for (int i = 0; i < st.length; i++) {
            stack += "\t at " + st[i].toString() + "\n";
        }

        return stack;
    }

    @Test
    @SmallTest
    public void validateBlockchainTokenTest() {
        BlockchainToken testToken = new BlockchainToken();
        java.lang.reflect.Field[] fields = testToken.getClass().getDeclaredFields();
        for (java.lang.reflect.Field f : fields) {
            try {
                java.lang.Class t = f.getType();
                java.lang.Object v = f.get(testToken);
                if (!t.isPrimitive()) {
                    String varName = f.getName();
                    if (varName.equals("contractAddress") || varName.equals("name")
                            || varName.equals("logo") || varName.equals("symbol")
                            || varName.equals("chainId")) {
                        continue;
                    }
                    if (v == null) {
                        String message =
                                "Check that "
                                        + varName
                                        + " is initialized everywhere\n"
                                        + "in Java files, where BlockchainToken object is created."
                                        + " It\n"
                                        + "could be safely added to the above if to skip that var"
                                        + " on checks\n"
                                        + "after that.";
                        fail(message);
                    }
                }
            } catch (Exception exc) {
                // Exception appears on private field members. We just skip them as we are
                // interested in public members of a mojom structure
            }
        }
        testToken.contractAddress = "";
        testToken.logo = "";
        testToken.name = "";
        testToken.symbol = "";
        testToken.chainId = "";
        testToken.coin = CoinType.ETH;
        try {
            java.nio.ByteBuffer byteBuffer = testToken.serialize();
            BlockchainToken testTokenDeserialized = BlockchainToken.deserialize(byteBuffer);
        } catch (Exception exc) {
            String message = "Check that a variable with a type in the exception below is\n"
                    + "initialized everywhere in Java files, where BlockchainToken object is\n"
                    + "created('git grep \"new BlockchainToken\"' inside src/brave).\n"
                    + "Initialisation of it could be safely added to the test to pass it,\n"
                    + "but only after all places where it's created are fixed.\n";
            fail(message + "\n" + getStackTrace(exc));
        }
    }

    @Test
    @SmallTest
    public void validateSwapQuoteParamsTest() {
        SwapQuoteParams testStruct = new SwapQuoteParams();
        java.lang.reflect.Field[] fields = testStruct.getClass().getDeclaredFields();
        for (java.lang.reflect.Field f : fields) {
            try {
                java.lang.Class t = f.getType();
                java.lang.Object v = f.get(testStruct);
                if (!t.isPrimitive()) {
                    String varName = f.getName();
                    if (varName.equals("fromAccountId")
                            || varName.equals("fromChainId")
                            || varName.equals("fromToken")
                            || varName.equals("fromAmount")
                            || varName.equals("toAccountId")
                            || varName.equals("toChainId")
                            || varName.equals("toToken")
                            || varName.equals("toAmount")
                            || varName.equals("slippagePercentage")
                            || varName.equals("routePriority")) {
                        continue;
                    }
                    if (v == null) {
                        String message =
                                "Check that "
                                        + varName
                                        + " is initialized everywhere\n"
                                        + "in Java files, where SwapQuoteParams object is created."
                                        + " It\n"
                                        + "could be safely added to the above if to skip that var"
                                        + " on checks\n"
                                        + "after that.";
                        fail(message);
                    }
                }
            } catch (Exception exc) {
                // Exception appears on private field members. We just skip them as we are
                // interested in public members of a mojom structure
            }
        }
        testStruct.fromAccountId = new AccountId();
        testStruct.fromAccountId.address = "";
        testStruct.fromAccountId.uniqueKey = "";
        testStruct.fromChainId = "";
        testStruct.fromToken = "";
        testStruct.fromAmount = "";
        testStruct.toAccountId = new AccountId();
        testStruct.toAccountId.address = "";
        testStruct.toAccountId.uniqueKey = "";
        testStruct.toChainId = "";
        testStruct.toToken = "";
        testStruct.toAmount = "";
        testStruct.slippagePercentage = "";
        testStruct.routePriority = RoutePriority.RECOMMENDED;

        try {
            java.nio.ByteBuffer byteBuffer = testStruct.serialize();
            SwapQuoteParams testStructDeserialized = SwapQuoteParams.deserialize(byteBuffer);
        } catch (Exception exc) {
            String message =
                    "Check that a variable with a type in the exception below is\n"
                        + "initialized everywhere in Java files, where SwapQuoteParams object is\n"
                        + "created('git grep \"new SwapQuoteParams\"' inside src/brave).\n"
                        + "Initialisation of it could be safely added to the test to pass it,\n"
                        + "but only after all places where it's created are fixed.\n";
            fail(message + "\n" + getStackTrace(exc));
        }
    }

    @Test
    @SmallTest
    public void validateTxDataTest() {
        TxData testStruct = new TxData();
        java.lang.reflect.Field[] fields = testStruct.getClass().getDeclaredFields();
        for (java.lang.reflect.Field f : fields) {
            try {
                java.lang.Class t = f.getType();
                java.lang.Object v = f.get(testStruct);
                if (!t.isPrimitive()) {
                    String varName = f.getName();
                    if (varName.equals("nonce") || varName.equals("gasPrice")
                            || varName.equals("gasLimit") || varName.equals("to")
                            || varName.equals("value") || varName.equals("data")
                            || varName.equals("signedTransaction")) {
                        continue;
                    }
                    if (v == null) {
                        String message =
                                "Check that "
                                        + varName
                                        + " is initialized everywhere\n"
                                        + "in Java files, where TxData object is created. It\n"
                                        + "could be safely added to the above if to skip that var"
                                        + " on checks\n"
                                        + "after that.";
                        fail(message);
                    }
                }
            } catch (Exception exc) {
                // Exception appears on private field members. We just skip them as we are
                // interested in public members of a mojom structure
            }
        }
        testStruct.nonce = "";
        testStruct.gasPrice = "";
        testStruct.gasLimit = "";
        testStruct.to = "";
        testStruct.value = "";
        testStruct.data = new byte[0];
        testStruct.signedTransaction = "";
        try {
            java.nio.ByteBuffer byteBuffer = testStruct.serialize();
            TxData testStructDeserialized = TxData.deserialize(byteBuffer);
        } catch (Exception exc) {
            String message = "Check that a variable with a type in the exception below is\n"
                    + "initialized everywhere in Java files, where TxData object is\n"
                    + "created('git grep \"new TxData\"' inside src/brave).\n"
                    + "Initialisation of it could be safely added to the test to pass it,\n"
                    + "but only after all places where it's created are fixed.\n";
            fail(message + "\n" + getStackTrace(exc));
        }
    }

    @Test
    @SmallTest
    public void validateGasEstimation1559Test() {
        GasEstimation1559 testStruct = new GasEstimation1559();
        java.lang.reflect.Field[] fields = testStruct.getClass().getDeclaredFields();
        for (java.lang.reflect.Field f : fields) {
            try {
                java.lang.Class t = f.getType();
                java.lang.Object v = f.get(testStruct);
                if (!t.isPrimitive()) {
                    String varName = f.getName();
                    if (varName.equals("slowMaxPriorityFeePerGas")
                            || varName.equals("slowMaxFeePerGas")
                            || varName.equals("avgMaxPriorityFeePerGas")
                            || varName.equals("avgMaxFeePerGas")
                            || varName.equals("fastMaxPriorityFeePerGas")
                            || varName.equals("fastMaxFeePerGas")
                            || varName.equals("baseFeePerGas")) {
                        continue;
                    }
                    if (v == null) {
                        String message =
                                "Check that "
                                        + varName
                                        + " is initialized everywhere\n"
                                        + "in Java files, where GasEstimation1559 object is"
                                        + " created. It\n"
                                        + "could be safely added to the above if to skip that var"
                                        + " on checks\n"
                                        + "after that.";
                        fail(message);
                    }
                }
            } catch (Exception exc) {
                // Exception appears on private field members. We just skip them as we are
                // interested in public members of a mojom structure
            }
        }
        testStruct.slowMaxPriorityFeePerGas = "";
        testStruct.slowMaxFeePerGas = "";
        testStruct.avgMaxPriorityFeePerGas = "";
        testStruct.avgMaxFeePerGas = "";
        testStruct.fastMaxPriorityFeePerGas = "";
        testStruct.fastMaxFeePerGas = "";
        testStruct.baseFeePerGas = "";
        try {
            java.nio.ByteBuffer byteBuffer = testStruct.serialize();
            GasEstimation1559 testStructDeserialized = GasEstimation1559.deserialize(byteBuffer);
        } catch (Exception exc) {
            String message = "Check that a variable with a type in the exception below is\n"
                    + "initialized everywhere in Java files, where GasEstimation1559 object is\n"
                    + "created('git grep \"new GasEstimation1559\"' inside src/brave).\n"
                    + "Initialisation of it could be safely added to the test to pass it,\n"
                    + "but only after all places where it's created are fixed.\n";
            fail(message + "\n" + getStackTrace(exc));
        }
    }

    @Test
    @SmallTest
    public void validateTxData1559Test() {
        TxData1559 testStruct = new TxData1559();
        java.lang.reflect.Field[] fields = testStruct.getClass().getDeclaredFields();
        for (java.lang.reflect.Field f : fields) {
            try {
                java.lang.Class t = f.getType();
                java.lang.Object v = f.get(testStruct);
                if (!t.isPrimitive()) {
                    String varName = f.getName();
                    if (varName.equals("baseData") || varName.equals("chainId")
                            || varName.equals("maxPriorityFeePerGas")
                            || varName.equals("maxFeePerGas") || varName.equals("gasEstimation")) {
                        continue;
                    }
                    if (v == null) {
                        String message =
                                "Check that "
                                        + varName
                                        + " is initialized everywhere\n"
                                        + "in Java files, where TxData1559 object is created. It\n"
                                        + "could be safely added to the above if to skip that var"
                                        + " on checks\n"
                                        + "after that.";
                        fail(message);
                    }
                }
            } catch (Exception exc) {
                // Exception appears on private field members. We just skip them as we are
                // interested in public members of a mojom structure
            }
        }
        testStruct.baseData = new TxData();
        testStruct.baseData.nonce = "";
        testStruct.baseData.gasPrice = "";
        testStruct.baseData.gasLimit = "";
        testStruct.baseData.to = "";
        testStruct.baseData.value = "";
        testStruct.baseData.data = new byte[0];
        testStruct.chainId = "";
        testStruct.maxPriorityFeePerGas = "";
        testStruct.maxFeePerGas = "";
        testStruct.gasEstimation = new GasEstimation1559();
        testStruct.gasEstimation.slowMaxPriorityFeePerGas = "";
        testStruct.gasEstimation.slowMaxFeePerGas = "";
        testStruct.gasEstimation.avgMaxPriorityFeePerGas = "";
        testStruct.gasEstimation.avgMaxFeePerGas = "";
        testStruct.gasEstimation.fastMaxPriorityFeePerGas = "";
        testStruct.gasEstimation.fastMaxFeePerGas = "";
        testStruct.gasEstimation.baseFeePerGas = "";
        try {
            java.nio.ByteBuffer byteBuffer = testStruct.serialize();
            TxData1559 testStructDeserialized = TxData1559.deserialize(byteBuffer);
        } catch (Exception exc) {
            String message = "Check that a variable with a type in the exception below is\n"
                    + "initialized everywhere in Java files, where TxData1559 object is\n"
                    + "created('git grep \"new TxData1559\"' inside src/brave).\n"
                    + "Initialisation of it could be safely added to the test to pass it,\n"
                    + "but only after all places where it's created are fixed.\n";
            fail(message + "\n" + getStackTrace(exc));
        }
    }

    @Test
    @SmallTest
    public void validateNetworkInfoTest() {
        NetworkInfo testStruct = new NetworkInfo();
        java.lang.reflect.Field[] fields = testStruct.getClass().getDeclaredFields();
        for (java.lang.reflect.Field f : fields) {
            try {
                java.lang.Class t = f.getType();
                java.lang.Object v = f.get(testStruct);
                if (!t.isPrimitive()) {
                    String varName = f.getName();
                    if (varName.equals("chainId") || varName.equals("chainName")
                            || varName.equals("blockExplorerUrls") || varName.equals("iconUrls")
                            || varName.equals("rpcEndpoints") || varName.equals("supportedKeyrings")
                            || varName.equals("activeRpcEndpointIndex") || varName.equals("symbol")
                            || varName.equals("symbolName") || varName.equals("isEip1559")) {
                        continue;
                    }
                    if (v == null) {
                        String message =
                                "Check that "
                                        + varName
                                        + " is initialized everywhere\n"
                                        + "in Java files, where NetworkInfo object is created. It\n"
                                        + "could be safely added to the above if to skip that var"
                                        + " on checks\n"
                                        + "after that.";
                        fail(message);
                    }
                }
            } catch (Exception exc) {
                // Exception appears on private field members. We just skip them as we are
                // interested in public members of a mojom structure
            }
        }
        testStruct.chainId = "";
        testStruct.supportedKeyrings = new int[0];
        testStruct.chainName = "";
        testStruct.blockExplorerUrls = new String[0];
        testStruct.iconUrls = new String[0];
        testStruct.rpcEndpoints = new Url[0];
        testStruct.activeRpcEndpointIndex = 0;
        testStruct.symbol = "";
        testStruct.symbolName = "";
        testStruct.coin = CoinType.ETH;
        try {
            java.nio.ByteBuffer byteBuffer = testStruct.serialize();
            NetworkInfo testStructDeserialized = NetworkInfo.deserialize(byteBuffer);
        } catch (Exception exc) {
            String message = "Check that a variable with a type in the exception below is\n"
                    + "initialized everywhere in Java files, where NetworkInfo object is\n"
                    + "created('git grep \"new NetworkInfo\"' inside src/brave).\n"
                    + "Initialisation of it could be safely added to the test to pass it,\n"
                    + "but only after all places where it's created are fixed.\n";
            fail(message + "\n" + getStackTrace(exc));
        }
    }

    @Test
    @SmallTest
    public void validateHasUnicode() {
        assertTrue(Validations.hasUnicode("Sign into \u202e EVIL"));
        assertFalse(Validations.hasUnicode("Sign into LIVE"));
    }

    @Test
    @SmallTest
    public void validateUnicodeEscape() {
        assertEquals(Validations.unicodeEscape("Sign into \u202e EVIL"), "Sign into \\u202e EVIL");
        assertEquals(Validations.unicodeEscape("Sign into \u00ff EVIL"), "Sign into \\u00ff EVIL");
        assertEquals(Validations.unicodeEscape("Sign into \u012e EVIL"), "Sign into \\u012e EVIL");
    }
}
