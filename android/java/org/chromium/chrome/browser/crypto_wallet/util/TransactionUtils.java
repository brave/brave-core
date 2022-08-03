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
}
