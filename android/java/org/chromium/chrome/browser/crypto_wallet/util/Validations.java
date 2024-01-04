/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.res.Resources;

import androidx.annotation.NonNull;

import org.chromium.base.Callbacks;
import org.chromium.base.ContextUtils;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;

import java.util.HashSet;
import java.util.Locale;

public class Validations {
    public static class SendToAccountAddress {
        private HashSet<String> mKnownContractAddresses;
        private static int VALID_ACCOUNT_ADDRESS_BYTE_LENGTH = 20;
        private boolean mIsKnowContracts;

        public SendToAccountAddress() {}

        public void validate(NetworkInfo selectedNetwork, KeyringService keyringService,
                BlockchainRegistry blockchainRegistry, BraveWalletService braveWalletService,
                AccountInfo fromAccount, String receiverAccountAddress,
                Callbacks.Callback2<String, Boolean> callback) {
            String senderAccountAddressLower = fromAccount.address.toLowerCase(Locale.ENGLISH);

            String receiverAccountAddressLower = receiverAccountAddress.toLowerCase(Locale.ENGLISH);

            Resources resources = ContextUtils.getApplicationContext().getResources();

            if (senderAccountAddressLower.equals(receiverAccountAddressLower)) {
                callback.call(resources.getString(R.string.wallet_same_address_error), true);
                return;
            }

            @CoinType.EnumType
            int coinType = fromAccount.accountId.coin;
            if (coinType == CoinType.SOL) {
                // TODO(apaymyshev):  why are we validating wallet's account?
                braveWalletService.isBase58EncodedSolanaPubkey(fromAccount.address, success -> {
                    String responseMsg = "";
                    if (!success) {
                        responseMsg = resources.getString(R.string.invalid_sol_address);
                    }
                    callback.call(responseMsg, success);
                });
            } else if (coinType == CoinType.FIL) {
                if (receiverAccountAddress.isEmpty()) {
                    return;
                }
                final boolean showErrorMessage = !isValidFilAddress(receiverAccountAddress);
                final String errorMessage =
                        showErrorMessage ? resources.getString(R.string.invalid_fil_address) : "";
                callback.call(errorMessage, showErrorMessage);
            } else {
                // Steps to validate:
                // 1. Valid hex string
                //      0x <20 bytes | 40 chars>
                // 2. Not equal to ours address
                // 3. Not one of token's contract addresses
                // 4. Has valid checksum

                String receiverAccountAddressUpper =
                        Utils.maybeHexStrToUpperCase(receiverAccountAddress);

                byte[] bytesReceiverAccountAddress =
                        Utils.hexStrToNumberArray(receiverAccountAddress);
                if (!receiverAccountAddress.isEmpty()
                        && bytesReceiverAccountAddress.length
                                != VALID_ACCOUNT_ADDRESS_BYTE_LENGTH) {
                    callback.call(resources.getString(R.string.wallet_not_valid_eth_address), true);
                    return;
                }

                mIsKnowContracts = false;
                if (mKnownContractAddresses == null) {
                    assert braveWalletService != null;
                    assert blockchainRegistry != null;
                    assert selectedNetwork != null;

                    TokenUtils.getAllTokensFiltered(braveWalletService, blockchainRegistry,
                            selectedNetwork, TokenUtils.TokenType.ALL, (tokens) -> {
                                fillKnowContracts(tokens);
                                checkForKnowContracts(
                                        receiverAccountAddressLower, callback, resources);
                            });
                } else {
                    checkForKnowContracts(receiverAccountAddressLower, callback, resources);
                }
                if (mIsKnowContracts) return;

                keyringService.getChecksumEthAddress(receiverAccountAddress, checksum_address -> {
                    if (receiverAccountAddress.equals(checksum_address)
                            || receiverAccountAddress.isEmpty()) {
                        callback.call("", false);
                    } else if (receiverAccountAddressLower.equals(receiverAccountAddress)
                            || receiverAccountAddressUpper.equals(receiverAccountAddress)) {
                        callback.call(
                                resources.getString(R.string.address_missing_checksum_warning),
                                false);
                    } else {
                        callback.call(
                                resources.getString(R.string.address_not_valid_checksum_error),
                                true);
                    }
                });
            }
        }

        /**
         * Checks if the Filecoin contract address provided is valid.
         * @param address Lowercase Filecoin contract address.
         * @return {@code true} if the Filecoin contract address is valid, {@code false} otherwise.
         */
        private boolean isValidFilAddress(@NonNull final String address) {
            if (!address.startsWith(BraveWalletConstants.FILECOIN_MAINNET)
                    && !address.startsWith(BraveWalletConstants.FILECOIN_TESTNET)) {
                return false;
            }
            // Secp256k have 41 address length and BLS keys have 86.
            return (address.length() == 41 || address.length() == 86);
        }

        private void checkForKnowContracts(String receiverAccountAddressLower,
                Callbacks.Callback2<String, Boolean> callback, Resources resources) {
            assert mKnownContractAddresses != null;
            if (mKnownContractAddresses.contains(receiverAccountAddressLower)) {
                callback.call(resources.getString(R.string.wallet_contract_address_error), true);
                mIsKnowContracts = true;
            } else {
                callback.call("", false);
                mIsKnowContracts = false;
            }
        }

        private void fillKnowContracts(BlockchainToken[] tokens) {
            mKnownContractAddresses = new HashSet<String>();
            for (BlockchainToken token : tokens) {
                if (token.contractAddress != null && !token.contractAddress.isEmpty()) {
                    mKnownContractAddresses.add(
                            token.contractAddress.toLowerCase(Locale.getDefault()));
                }
            }
        }
    }

    public static boolean hasUnicode(String str) {
        for (int i = 0; i < str.length(); i++) {
            if (str.codePointAt(i) > 127) {
                return true;
            }
        }

        return false;
    }

    public static String unicodeEscape(String str) {
        String res = "";

        for (int i = 0; i < str.length(); i++) {
            if (str.codePointAt(i) > 127) {
                String hexStr = Integer.toHexString(str.codePointAt(i));
                while (hexStr.length() < 4) {
                    hexStr = "0" + hexStr;
                }
                res += "\\u" + hexStr;
            } else {
                res += str.charAt(i);
            }
        }

        return res;
    }
}
