/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertThrows;
import static org.mockito.Mockito.when;

import androidx.fragment.app.FragmentActivity;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.fragments.ApproveTxBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.WalletFragmentCallback;
import org.chromium.chrome.browser.crypto_wallet.listeners.TransactionListener;

import java.lang.reflect.Field;
import java.util.Collections;
import java.util.List;

/** Tests for {@link ApproveTxBottomSheetDialogFragment}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ApproveTxBottomSheetDialogFragmentTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private WalletModel mWalletModel;
    @Mock private KeyringModel mKeyringModel;

    @Test
    public void onAttach_requiresTransactionListener() {
        when(mWalletModel.getKeyringModel()).thenReturn(mKeyringModel);

        WalletOnlyActivity activity =
                Robolectric.buildActivity(WalletOnlyActivity.class).setup().get();
        activity.setWalletModel(mWalletModel);

        ApproveTxBottomSheetDialogFragment fragment = new ApproveTxBottomSheetDialogFragment();

        IllegalStateException thrown =
                assertThrows(
                        IllegalStateException.class,
                        () ->
                                activity.getSupportFragmentManager()
                                        .beginTransaction()
                                        .add(fragment, "tag")
                                        .commitNow());
        // Sanity check for the error message, ensuring the new requirement is documented.
        assertThrowsContains("TransactionListener", thrown);
    }

    @Test
    public void onAttach_storesTransactionStateFromListener() throws Exception {
        when(mWalletModel.getKeyringModel()).thenReturn(mKeyringModel);

        TransactionInfo txInfo = new TransactionInfo();
        List<TransactionInfo> pending = Collections.singletonList(new TransactionInfo());

        TransactionHostActivity activity =
                Robolectric.buildActivity(TransactionHostActivity.class).setup().get();
        activity.setWalletModel(mWalletModel);
        activity.setCurrentTransaction(txInfo);
        activity.setPendingTransactions(pending);

        TestApproveTxBottomSheetDialogFragment fragment =
                new TestApproveTxBottomSheetDialogFragment();
        activity.getSupportFragmentManager().beginTransaction().add(fragment, "tag").commitNow();

        assertSame(txInfo, fragment.getCurrentTransactionForTest());
        assertEquals(pending, fragment.getPendingTransactionsForTest());
    }

    private static void assertThrowsContains(String expected, IllegalStateException thrown) {
        if (thrown.getMessage() == null || !thrown.getMessage().contains(expected)) {
            throw new AssertionError(
                    "Expected message to contain '"
                            + expected
                            + "' but was: "
                            + thrown.getMessage());
        }
    }

    /** Activity that provides only the wallet model. */
    public static class WalletOnlyActivity extends FragmentActivity
            implements WalletFragmentCallback {
        private WalletModel mWalletModel;

        void setWalletModel(WalletModel walletModel) {
            mWalletModel = walletModel;
        }

        @Override
        public WalletModel getWalletModel() {
            return mWalletModel;
        }
    }

    /** Activity that implements both wallet and transaction contracts. */
    public static class TransactionHostActivity extends WalletOnlyActivity
            implements TransactionListener {
        private List<TransactionInfo> mPendingTransactions = Collections.emptyList();
        private TransactionInfo mCurrentTransaction;

        void setPendingTransactions(List<TransactionInfo> pendingTransactions) {
            mPendingTransactions = pendingTransactions;
        }

        void setCurrentTransaction(TransactionInfo currentTransaction) {
            mCurrentTransaction = currentTransaction;
        }

        @Override
        public void onNextTransaction() {}

        @Override
        public void onRejectAllTransactions() {}

        @Override
        public void onCancel() {}

        @Override
        public List<TransactionInfo> getPendingTransactions() {
            return mPendingTransactions;
        }

        @Override
        public TransactionInfo getCurrentTransaction() {
            return mCurrentTransaction;
        }
    }

    /**
     * Test-specific fragment that skips UI initialisation and exposes the listener state populated
     * during {@link ApproveTxBottomSheetDialogFragment#onAttach(android.content.Context)}.
     */
    public static class TestApproveTxBottomSheetDialogFragment
            extends ApproveTxBottomSheetDialogFragment {
        @Override
        public void onViewCreated(android.view.View view, android.os.Bundle savedInstanceState) {
            // Skip heavy UI initialisation for tests.
        }

        @Override
        public android.view.View onCreateView(
                android.view.LayoutInflater inflater,
                android.view.ViewGroup container,
                android.os.Bundle savedInstanceState) {
            return new android.view.View(inflater.getContext());
        }

        TransactionInfo getCurrentTransactionForTest() throws Exception {
            Field field = ApproveTxBottomSheetDialogFragment.class.getDeclaredField("mTxInfo");
            field.setAccessible(true);
            return (TransactionInfo) field.get(this);
        }

        @SuppressWarnings("unchecked")
        List<TransactionInfo> getPendingTransactionsForTest() throws Exception {
            Field field =
                    ApproveTxBottomSheetDialogFragment.class.getDeclaredField(
                            "mPendingTransactions");
            field.setAccessible(true);
            return (List<TransactionInfo>) field.get(this);
        }
    }
}
