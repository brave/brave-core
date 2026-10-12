/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import static org.junit.Assert.assertEquals;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.brave_wallet.mojom.CardanoSignData;
import org.chromium.brave_wallet.mojom.EthSignTypedData;
import org.chromium.brave_wallet.mojom.EthStandardSignData;
import org.chromium.brave_wallet.mojom.SignDataUnion;
import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.brave_wallet.mojom.SiweMessage;
import org.chromium.brave_wallet.mojom.SolanaSignData;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity.ActivityType;

/**
 * Tests the mapping from a sign request to the panel that displays it. The browser only processes
 * the request at the head of the queue, so a panel left showing a different request would approve a
 * message the user never saw. Both sign panels resolve the panel through this mapping and hand the
 * request over when the answer is not themselves, which also means the mapping must be total.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class DappsModelTest {
    @Test
    public void getSignMessagePanel_siweData_usesSignInPanel() {
        SignDataUnion signData = new SignDataUnion();
        signData.setEthSiweData(new SiweMessage());

        assertEquals(ActivityType.SIWE_MESSAGE, panelFor(signData));
    }

    @Test
    public void getSignMessagePanel_typedData_usesSignMessagePanel() {
        SignDataUnion signData = new SignDataUnion();
        signData.setEthSignTypedData(new EthSignTypedData());

        // The panel that carries the risk warning: EIP-712 typed data can grant token allowances.
        assertEquals(ActivityType.SIGN_MESSAGE, panelFor(signData));
    }

    @Test
    public void getSignMessagePanel_standardData_usesSignMessagePanel() {
        SignDataUnion signData = new SignDataUnion();
        signData.setEthStandardSignData(new EthStandardSignData());

        assertEquals(ActivityType.SIGN_MESSAGE, panelFor(signData));
    }

    @Test
    public void getSignMessagePanel_solanaData_usesSignMessagePanel() {
        SignDataUnion signData = new SignDataUnion();
        signData.setSolanaSignData(new SolanaSignData());

        assertEquals(ActivityType.SIGN_MESSAGE, panelFor(signData));
    }

    @Test
    public void getSignMessagePanel_cardanoData_usesSignMessagePanel() {
        SignDataUnion signData = new SignDataUnion();
        signData.setCardanoSignData(new CardanoSignData());

        assertEquals(ActivityType.SIGN_MESSAGE, panelFor(signData));
    }

    private static ActivityType panelFor(SignDataUnion signData) {
        SignMessageRequest request = new SignMessageRequest();
        request.signData = signData;
        return DappsModel.getSignMessagePanel(request);
    }
}
