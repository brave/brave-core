/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;

public class TransactionUtils {

    public static boolean isSolanaDappTransaction(TransactionInfo txInfo) {
        if(txInfo == null) return false;
        return txInfo.txType == TransactionType.SOLANA_DAPP_SIGN_AND_SEND_TRANSACTION ||
        txInfo.txType == TransactionType.SOLANA_DAPP_SIGN_TRANSACTION;
    }

    public static boolean isSPLTransaction (TransactionInfo txInfo){
        if(txInfo == null) return false;
        return txInfo.txType == TransactionType.SOLANA_SPL_TOKEN_TRANSFER ||
      txInfo.txType == TransactionType.SOLANA_SPL_TOKEN_TRANSFER_WITH_ASSOCIATED_TOKEN_ACCOUNT_CREATION;    
    }

    public static boolean isSolTransaction(TransactionInfo txInfo){
        if(txInfo == null) return false;
        return txInfo.txType == TransactionType.SOLANA_SYSTEM_TRANSFER ||
      isSPLTransaction(txInfo) || isSolanaDappTransaction(txInfo);
    }

    public static int getCoinType(TransactionInfo txInfo){
        if(txInfo == null) return CoinType.ETH;
        return isSolTransaction(txInfo)? CoinType.SOL : CoinType.ETH;
    }
}