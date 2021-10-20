/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_wallet;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import androidx.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

@RunWith(ChromeJUnit4ClassRunner.class)
public class BraveWalletUtilsTest {
    @Test
    @SmallTest
    public void fromHexWeiTest() {
        assertEquals(Utils.fromHexWei("0x2B5E3AF16B0000000"), 50, 0.001);
        assertEquals(Utils.fromHexWei("0x4563918244F40000"), 5, 0.001);
        assertEquals(Utils.fromHexWei("0x6F05B59D3B20000"), 0.5, 0.001);
        assertEquals(Utils.fromHexWei("0xB1A2BC2EC50000"), 0.05, 0.001);
        assertEquals(Utils.fromHexWei("0x0"), 0, 0.001);
        assertEquals(Utils.fromHexWei(""), 0, 0.001);
    }

    @Test
    @SmallTest
    public void toHexWeiTest() {
        assertEquals(Utils.toHexWei("5.2"), "0x482a1c7300080000");
        assertEquals(Utils.toHexWei("5"), "0x4563918244f40000");
        assertEquals(Utils.toHexWei("0.5"), "0x6f05b59d3b20000");
        assertEquals(Utils.toHexWei("0.05"), "0xb1a2bc2ec50000");
        assertEquals(Utils.toHexWei(""), "0x0");
    }

    @Test
    @SmallTest
    public void fromWeiTest() {
        assertEquals(Utils.fromWei("50000000000000000000"), 50, 0.001);
        assertEquals(Utils.fromWei("5000000000000000000"), 5, 0.001);
        assertEquals(Utils.fromWei("500000000000000000"), 0.5, 0.001);
        assertEquals(Utils.fromWei("50000000000000000"), 0.05, 0.001);
        assertEquals(Utils.fromWei(null), 0, 0.001);
        assertEquals(Utils.fromWei(""), 0, 0.001);
    }

    @Test
    @SmallTest
    public void toWei() {
        assertEquals(Utils.toWei("50"), "50000000000000000000");
        assertEquals(Utils.toWei("5"), "5000000000000000000");
        assertEquals(Utils.toWei("0.5"), "500000000000000000");
        assertEquals(Utils.toWei("0.05"), "50000000000000000");
        assertEquals(Utils.toWei(""), "0");
    }
}
