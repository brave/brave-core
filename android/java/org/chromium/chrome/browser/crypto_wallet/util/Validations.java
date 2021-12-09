/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.res.Resources;

import org.chromium.base.ContextUtils;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.ErcToken;
import org.chromium.brave_wallet.mojom.ErcTokenRegistry;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.mojo.bindings.Callbacks;

import java.util.HashSet;
import java.util.Locale;

public class Validations {
    public static class SendToAccountAddress {
        private HashSet<String> mKnownContractAddresses;
        private static int VALID_ACCOUNT_ADDRESS_BYTE_LENGTH = 20;

        public SendToAccountAddress() {}

        public void validate(String chainId, ErcTokenRegistry ercTokenRegistry,
                BraveWalletService braveWalletService, String senderAccountAddress,
                String receiverAccountAddress, Callbacks.Callback1<String> callback) {
            // Steps to validate:
            // 1. Valid hex string
            //      0x <20 bytes | 40 chars>
            // 2. Not equal to ours address
            // 3. Not one of token's contract addresses

            String senderAccountAddressLower =
                    senderAccountAddress.toLowerCase(Locale.getDefault());

            String receiverAccountAddressLower =
                    receiverAccountAddress.toLowerCase(Locale.getDefault());

            Resources resources = ContextUtils.getApplicationContext().getResources();

            byte[] bytesReceiverAccountAddress = Utils.hexStrToNumberArray(receiverAccountAddress);
            if (bytesReceiverAccountAddress.length != VALID_ACCOUNT_ADDRESS_BYTE_LENGTH) {
                callback.call(resources.getString(R.string.wallet_not_valid_eth_address));
                return;
            }

            if (senderAccountAddressLower.equals(receiverAccountAddressLower)) {
                callback.call(resources.getString(R.string.wallet_same_address_error));
                return;
            }

            if (mKnownContractAddresses == null) {
                assert braveWalletService != null;
                assert ercTokenRegistry != null;
                assert chainId != null && !chainId.isEmpty();

                TokenUtils.getAllTokensFiltered(
                        braveWalletService, ercTokenRegistry, chainId, (tokens) -> {
                            tokens = Utils.fixupTokensRegistry(tokens, chainId);
                            fillKnowContracts(tokens);
                            checkForKnowContracts(receiverAccountAddressLower, callback, resources);
                        });
            } else {
                checkForKnowContracts(receiverAccountAddressLower, callback, resources);
            }
        }

        private void checkForKnowContracts(String receiverAccountAddressLower,
                Callbacks.Callback1<String> callback, Resources resources) {
            assert mKnownContractAddresses != null;
            if (mKnownContractAddresses.contains(receiverAccountAddressLower)) {
                callback.call(resources.getString(R.string.wallet_contract_address_error));
            } else {
                callback.call("");
            }
        }

        void fillKnowContracts(ErcToken[] tokens) {
            mKnownContractAddresses = new HashSet<String>();
            for (ErcToken token : tokens) {
                if (token.contractAddress != null && !token.contractAddress.isEmpty()) {
                    mKnownContractAddresses.add(
                            token.contractAddress.toLowerCase(Locale.getDefault()));
                }
            }
        }
    }
}
