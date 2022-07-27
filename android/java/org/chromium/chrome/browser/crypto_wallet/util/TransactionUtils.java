/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.FilTxData;
import org.chromium.brave_wallet.mojom.SolanaTxData;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.brave_wallet.mojom.TxDataUnion;

public class TransactionUtils {
    public static @CoinType.EnumType int getCoinFromTxDataUnion(TxDataUnion txDataUnion) {
        if (txDataUnion.which() == TxDataUnion.Tag.FilTxData)
            return CoinType.FIL;
        else if (txDataUnion.which() == TxDataUnion.Tag.SolanaTxData)
            return CoinType.SOL;
        else
            return CoinType.ETH;
    }

    public static TxData1559 safeEthTxData1559(TxDataUnion txDataUnion) {
        try {
            return txDataUnion.getEthTxData1559();
        } catch (AssertionError e) {
            e.printStackTrace();
        }
        return null;
    }

    public static SolanaTxData safeSolTxData1559(TxDataUnion txDataUnion) {
        try {
            return txDataUnion.getSolanaTxData();
        } catch (AssertionError e) {
            e.printStackTrace();
        }
        return null;
    }

    public static FilTxData safeFilTxData1559(TxDataUnion txDataUnion) {
        try {
            return txDataUnion.getFilTxData();
        } catch (AssertionError e) {
            e.printStackTrace();
        }
        return null;
    }
}
