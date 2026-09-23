/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import androidx.annotation.IntegerRes;

import org.chromium.brave_wallet.mojom.CoinType;

import java.io.Serializable;

public class CryptoAccountTypeInfo implements Serializable {
    private String desc;
    private String name;
    private int coinType;
    private @IntegerRes int icon;

    public CryptoAccountTypeInfo(
            String desc, String name, @CoinType.EnumType int coinType, int icon) {
        this.desc = desc;
        this.name = name;
        this.coinType = coinType;
        this.icon = icon;
    }

    public String getDesc() {
        return desc;
    }

    public void setDesc(String desc) {
        this.desc = desc;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public @CoinType.EnumType int getCoinType() {
        return coinType;
    }

    public void setCoinType(@CoinType.EnumType int coinType) {
        this.coinType = coinType;
    }

    public int getIcon() {
        return icon;
    }

    public void setIcon(int icon) {
        this.icon = icon;
    }
}
