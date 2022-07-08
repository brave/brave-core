package org.chromium.chrome.browser.app.domain;

import android.content.Context;

import androidx.lifecycle.LiveData;

import org.chromium.brave_wallet.mojom.CoinType;

public interface CryptoSharedActions {
    void updateCoinType();
    void updateCoinAccountNetworkInfo(@CoinType.EnumType int coin);
}
