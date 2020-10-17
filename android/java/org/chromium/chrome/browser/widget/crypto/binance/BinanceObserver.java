/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.widget.crypto.binance;

public interface BinanceObserver {
    default public void OnGetAccessToken(boolean isSuccess){};
    default public void OnGetAccountBalances(String jsonBalances, boolean isSuccess){};
    default public void OnGetConvertQuote(
            String quoteId, String quotePrice, String totalFee, String totalAmount){};
    default public void OnGetCoinNetworks(String jsonNetworks){};
    default public void OnGetDepositInfo(
            String depositAddress, String depositTag, boolean isSuccess){};
    default public void OnConfirmConvert(boolean isSuccess, String message){};
    default public void OnGetConvertAssets(String jsonAssets){};
    default public void OnRevokeToken(boolean isSuccess){};
}