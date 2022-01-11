/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.res.Resources;

import org.chromium.base.ContextUtils;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.mojo.bindings.Callbacks;

import java.util.HashSet;
import java.util.Locale;

public class Validations {
    public static class SendToAccountAddress {
        private HashSet<String> mKnownContractAddresses;
        private static int VALID_ACCOUNT_ADDRESS_BYTE_LENGTH = 20;
        private boolean mIsKnowContracts;

        public SendToAccountAddress() {}

        public void validate(String chainId, KeyringService keyringService,
                BlockchainRegistry blockchainRegistry, BraveWalletService braveWalletService,
                String senderAccountAddress, String receiverAccountAddress,
                Callbacks.Callback2<String, Boolean> callback) {
            // Steps to validate:
            // 1. Valid hex string
            //      0x <20 bytes | 40 chars>
            // 2. Not equal to ours address
            // 3. Not one of token's contract addresses
            // 4. Has valid checksum

            String senderAccountAddressLower =
                    senderAccountAddress.toLowerCase(Locale.getDefault());

            String receiverAccountAddressLower =
                    receiverAccountAddress.toLowerCase(Locale.getDefault());
            String receiverAccountAddressUpper =
                    Utils.maybeHexStrToUpperCase(receiverAccountAddress);

            Resources resources = ContextUtils.getApplicationContext().getResources();

            byte[] bytesReceiverAccountAddress = Utils.hexStrToNumberArray(receiverAccountAddress);
            if (bytesReceiverAccountAddress.length != VALID_ACCOUNT_ADDRESS_BYTE_LENGTH) {
                callback.call(resources.getString(R.string.wallet_not_valid_eth_address), true);
                return;
            }

            if (senderAccountAddressLower.equals(receiverAccountAddressLower)) {
                callback.call(resources.getString(R.string.wallet_same_address_error), true);
                return;
            }

            mIsKnowContracts = false;
            if (mKnownContractAddresses == null) {
                assert braveWalletService != null;
                assert blockchainRegistry != null;
                assert chainId != null && !chainId.isEmpty();

                TokenUtils.getAllTokensFiltered(
                        braveWalletService, blockchainRegistry, chainId, (tokens) -> {
                            tokens = Utils.fixupTokensRegistry(tokens, chainId);
                            fillKnowContracts(tokens);
                            checkForKnowContracts(receiverAccountAddressLower, callback, resources);
                        });
            } else {
                checkForKnowContracts(receiverAccountAddressLower, callback, resources);
            }
            if (mIsKnowContracts) return;

            keyringService.getChecksumEthAddress(receiverAccountAddress, checksum_address -> {
                if (receiverAccountAddress.equals(checksum_address)) {
                    callback.call("", false);
                } else if (receiverAccountAddressLower.equals(receiverAccountAddress)
                        || receiverAccountAddressUpper.equals(receiverAccountAddress)) {
                    callback.call(
                            resources.getString(R.string.address_missing_checksum_warning), false);
                } else {
                    callback.call(
                            resources.getString(R.string.address_not_valid_checksum_error), true);
                }
            });
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

        void fillKnowContracts(BlockchainToken[] tokens) {
            mKnownContractAddresses = new HashSet<String>();
            for (BlockchainToken token : tokens) {
                if (token.contractAddress != null && !token.contractAddress.isEmpty()) {
                    mKnownContractAddresses.add(
                            token.contractAddress.toLowerCase(Locale.getDefault()));
                }
            }
        }
    }
}
