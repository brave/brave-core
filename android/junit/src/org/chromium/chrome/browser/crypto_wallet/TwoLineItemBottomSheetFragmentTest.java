/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.mockito.Mockito.when;

import androidx.fragment.app.FragmentActivity;
import androidx.recyclerview.widget.RecyclerView;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.TwoLineItemRecyclerViewAdapter.TwoLineItemText;
import org.chromium.chrome.browser.crypto_wallet.fragments.TwoLineItemBottomSheetFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.WalletFragmentCallback;

import java.util.Collections;

/** Tests for {@link TwoLineItemBottomSheetFragment}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class TwoLineItemBottomSheetFragmentTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private WalletModel mWalletModel;
    @Mock private KeyringModel mKeyringModel;

    @Test
    public void setItems_beforeViewCreated_bindsAdapter() {
        when(mWalletModel.getKeyringModel()).thenReturn(mKeyringModel);

        TestWalletActivity activity =
                Robolectric.buildActivity(TestWalletActivity.class).setup().get();
        activity.setWalletModel(mWalletModel);

        TwoLineItemBottomSheetFragment fragment = new TwoLineItemBottomSheetFragment();
        fragment.setItems(Collections.singletonList(new TwoLineItemText("title", "subtitle")));

        activity.getSupportFragmentManager().beginTransaction().add(fragment, "tag").commitNow();

        RecyclerView recyclerView = fragment.getView().findViewById(R.id.frag_two_line_sheet_list);
        assertNotNull(recyclerView.getAdapter());
        assertEquals(1, recyclerView.getAdapter().getItemCount());
    }

    /** Minimal activity that provides a {@link WalletModel} to wallet fragments. */
    public static class TestWalletActivity extends FragmentActivity
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
}
