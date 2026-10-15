/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.domain;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.BraveWalletService.GetPendingDecryptRequests_Response;
import org.chromium.brave_wallet.mojom.BraveWalletService.GetPendingGetEncryptionPublicKeyRequests_Response;
import org.chromium.brave_wallet.mojom.BraveWalletService.GetPendingSignSolTransactionsRequests_Response;
import org.chromium.brave_wallet.mojom.CardanoSignData;
import org.chromium.brave_wallet.mojom.DecryptRequest;
import org.chromium.brave_wallet.mojom.EthSignTypedData;
import org.chromium.brave_wallet.mojom.EthStandardSignData;
import org.chromium.brave_wallet.mojom.GetEncryptionPublicKeyRequest;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.SignDataUnion;
import org.chromium.brave_wallet.mojom.SignMessageRequest;
import org.chromium.brave_wallet.mojom.SignSolTransactionsRequest;
import org.chromium.brave_wallet.mojom.SiweMessage;
import org.chromium.brave_wallet.mojom.SolanaSignData;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletDAppsActivity.ActivityType;

import java.util.concurrent.atomic.AtomicReference;

/**
 * Tests how {@link DappsModel} tells a DApps panel what to do next: which panel displays a sign
 * request, and whether any request is left once the panel has processed one.
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

        // The generic panel warns before typed data is signed, since it can grant allowances.
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

    @Test
    public void processPublicEncryptionKey_reportsWhetherRequestsRemain() {
        assertTrue(processPublicEncryptionKey(new GetEncryptionPublicKeyRequest[1]));
        assertFalse(processPublicEncryptionKey(new GetEncryptionPublicKeyRequest[0]));
    }

    @Test
    public void processDecryptRequest_reportsWhetherRequestsRemain() {
        assertTrue(processDecryptRequest(new DecryptRequest[1]));
        assertFalse(processDecryptRequest(new DecryptRequest[0]));
    }

    @Test
    public void notifySignSolTransactionsRequestProcessed_reportsWhetherRequestsRemain() {
        assertTrue(notifySignSolTransactionsRequestProcessed(new SignSolTransactionsRequest[1]));
        assertFalse(notifySignSolTransactionsRequestProcessed(new SignSolTransactionsRequest[0]));
    }

    private static ActivityType panelFor(SignDataUnion signData) {
        SignMessageRequest request = new SignMessageRequest();
        request.signData = signData;
        return DappsModel.getSignMessagePanel(request);
    }

    private static Boolean processPublicEncryptionKey(GetEncryptionPublicKeyRequest[] pending) {
        BraveWalletService service = mock(BraveWalletService.class);
        doAnswer(
                        invocation -> {
                            GetPendingGetEncryptionPublicKeyRequests_Response callback =
                                    invocation.getArgument(0);
                            callback.call(pending);
                            return null;
                        })
                .when(service)
                .getPendingGetEncryptionPublicKeyRequests(any());
        AtomicReference<Boolean> hasMoreRequests = new AtomicReference<>();
        newModel(service).processPublicEncryptionKey("id", true, hasMoreRequests::set);
        return hasMoreRequests.get();
    }

    private static Boolean processDecryptRequest(DecryptRequest[] pending) {
        BraveWalletService service = mock(BraveWalletService.class);
        doAnswer(
                        invocation -> {
                            GetPendingDecryptRequests_Response callback = invocation.getArgument(0);
                            callback.call(pending);
                            return null;
                        })
                .when(service)
                .getPendingDecryptRequests(any());
        AtomicReference<Boolean> hasMoreRequests = new AtomicReference<>();
        newModel(service).processDecryptRequest("id", true, hasMoreRequests::set);
        return hasMoreRequests.get();
    }

    private static Boolean notifySignSolTransactionsRequestProcessed(
            SignSolTransactionsRequest[] pending) {
        BraveWalletService service = mock(BraveWalletService.class);
        doAnswer(
                        invocation -> {
                            GetPendingSignSolTransactionsRequests_Response callback =
                                    invocation.getArgument(0);
                            callback.call(pending);
                            return null;
                        })
                .when(service)
                .getPendingSignSolTransactionsRequests(any());
        AtomicReference<Boolean> hasMoreRequests = new AtomicReference<>();
        newModel(service)
                .notifySignSolTransactionsRequestProcessed(
                        true, new SignSolTransactionsRequest(), hasMoreRequests::set);
        return hasMoreRequests.get();
    }

    private static DappsModel newModel(BraveWalletService braveWalletService) {
        // The constructor only uses the keyring service, to observe it.
        return new DappsModel(
                /* jsonRpcService= */ null,
                braveWalletService,
                mock(KeyringService.class),
                /* pendingTxHelper= */ null);
    }
}
