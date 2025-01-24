/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.presenters;

import android.text.TextUtils;

import java.math.BigDecimal;
import java.math.RoundingMode;

public class Amount {
    private BigDecimal mValue;

    public Amount(BigDecimal value) {
        if (value == null) {
            this.mValue = BigDecimal.ZERO;
            return;
        }
        this.mValue = value;
    }

    public Amount(String value) {
        if (TextUtils.isEmpty(value)) {
            this.mValue = BigDecimal.ZERO;
            return;
        }
        this.mValue = new BigDecimal(value);
    }

    public Amount(double value) {
        this.mValue = new BigDecimal(value);
    }

    public Amount add(BigDecimal value) {
        BigDecimal safeVal = safeVal(value);
        mValue = mValue.add(safeVal);
        return this;
    }

    public Amount add(Amount value) {
        BigDecimal safeVal = safeVal(value);
        mValue = mValue.add(safeVal);
        return this;
    }

    // Default max decimal precision is 8
    public String toStringFormat() {
        return toStringFormat(8);
    }

    public String toStringFormat(int decimalLength) {
        return mValue.setScale(decimalLength, RoundingMode.HALF_UP)
                .stripTrailingZeros()
                .toPlainString();
    }

    public String toHex() {
        if (mValue.signum() == 0) return "0x0";
        return "0x" + mValue.toBigInteger().toString(16);
    }

    private static BigDecimal safeVal(BigDecimal value) {
        return (value == null) ? BigDecimal.ZERO : value;
    }

    private static BigDecimal safeVal(Amount value) {
        return (value != null && value.mValue != null) ? value.mValue : BigDecimal.ZERO;
    }

    public BigDecimal getValue() {
        return BigDecimal.valueOf(mValue.doubleValue());
    }
}
