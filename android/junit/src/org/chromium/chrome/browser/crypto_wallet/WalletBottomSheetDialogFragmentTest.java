/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet;

import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertThrows;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentTransaction;

import org.junit.After;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.Robolectric;
import org.robolectric.android.controller.ActivityController;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.app.domain.KeyringModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.fragments.WalletBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.WalletFragmentCallback;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserverImpl;

/** Tests for {@link WalletBottomSheetDialogFragment}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class WalletBottomSheetDialogFragmentTest {
    @Rule public final MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private WalletModel mWalletModel;
    @Mock private KeyringModel mKeyringModel;

    private ActivityController<?> mActivityController;

    @After
    public void tearDown() {
        if (mActivityController != null) {
            mActivityController.pause().stop().destroy();
        }
    }

    @Test
    public void onAttach_registersKeyringObserver() {
        when(mWalletModel.getKeyringModel()).thenReturn(mKeyringModel);

        mActivityController = Robolectric.buildActivity(TestWalletActivity.class).setup();
        TestWalletActivity activity = (TestWalletActivity) mActivityController.get();
        activity.setWalletModel(mWalletModel);

        TestWalletBottomSheetFragment fragment = new TestWalletBottomSheetFragment();
        activity.getSupportFragmentManager().beginTransaction().add(fragment, "tag").commitNow();

        verify(mWalletModel).getKeyringModel();
        verify(mKeyringModel).registerKeyringObserver(any(KeyringServiceObserverImpl.class));
        assertSame(mWalletModel, fragment.getAttachedWalletModel());
    }

    @Test
    public void onAttach_withoutCallback_throws() {
        mActivityController = Robolectric.buildActivity(FragmentActivity.class).setup();
        FragmentActivity activity = (FragmentActivity) mActivityController.get();

        TestWalletBottomSheetFragment fragment = new TestWalletBottomSheetFragment();
        FragmentTransaction fragmentTransaction =
                activity.getSupportFragmentManager().beginTransaction().add(fragment, "tag");
        assertThrows(IllegalStateException.class, fragmentTransaction::commitNow);
    }

    /** Simple fragment that exposes {@link WalletBottomSheetDialogFragment#getWalletModel()}. */
    public static class TestWalletBottomSheetFragment extends WalletBottomSheetDialogFragment {
        WalletModel getAttachedWalletModel() {
            return getWalletModel();
        }
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
