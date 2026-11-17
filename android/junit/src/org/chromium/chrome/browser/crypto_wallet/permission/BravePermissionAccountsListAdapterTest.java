/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.permission;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.view.ContextThemeWrapper;
import android.widget.FrameLayout;

import androidx.test.core.app.ApplicationProvider;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.permission.BravePermissionAccountsListAdapter.Mode;
import org.chromium.chrome.browser.crypto_wallet.permission.BravePermissionAccountsListAdapter.ViewHolder;

/**
 * Tests for {@link BravePermissionAccountsListAdapter}, asserting that the visibly checked accounts
 * always match the ones returned by {@code getCheckedAccounts()} when rows get recycled.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BravePermissionAccountsListAdapterTest {
    private static final int A = 0;
    private static final int B = 1;
    private static final int C = 2;

    private FrameLayout mParent;
    private AccountInfo[] mAccounts;

    @Before
    public void setUp() {
        Context context =
                new ContextThemeWrapper(
                        ApplicationProvider.getApplicationContext(),
                        R.style.Theme_BrowserUI_DayNight);
        mParent = new FrameLayout(context);
        mAccounts =
                new AccountInfo[] {
                    createAccount("0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
                    createAccount("0xbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"),
                    createAccount("0xcccccccccccccccccccccccccccccccccccccccc")
                };
    }

    @Test
    public void multipleSelection_recycledRowDoesNotCarryCheckedState() {
        BravePermissionAccountsListAdapter adapter = createAdapter(Mode.MULTIPLE_ACCOUNT_SELECTION);
        ViewHolder holder = adapter.onCreateViewHolder(mParent, 0);

        adapter.bindViewHolder(holder, A);
        holder.accountCheck.setChecked(true);
        assertCheckedAccounts(adapter, A);

        // Scrolling recycles the row of account A for account B.
        adapter.bindViewHolder(holder, B);
        assertFalse(holder.accountCheck.isChecked());
        assertCheckedAccounts(adapter, A);

        // Toggling B must not alter A.
        holder.accountCheck.setChecked(true);
        assertCheckedAccounts(adapter, A, B);
        holder.accountCheck.setChecked(false);
        assertCheckedAccounts(adapter, A);

        adapter.bindViewHolder(holder, C);
        assertFalse(holder.accountCheck.isChecked());
        holder.accountCheck.setChecked(true);
        assertCheckedAccounts(adapter, A, C);

        // Scrolling back shows A still checked, and it can be cleared.
        adapter.bindViewHolder(holder, A);
        assertTrue(holder.accountCheck.isChecked());
        assertCheckedAccounts(adapter, A, C);
        holder.accountCheck.setChecked(false);
        assertCheckedAccounts(adapter, C);
    }

    @Test
    public void singleSelection_offScreenSelectionIsReplaced() {
        BravePermissionAccountsListAdapter adapter = createAdapter(Mode.SINGLE_ACCOUNT_SELECTION);
        ViewHolder holderA = adapter.onCreateViewHolder(mParent, 0);
        ViewHolder holderB = adapter.onCreateViewHolder(mParent, 0);

        adapter.bindViewHolder(holderA, A);
        holderA.accountRadioButton.setChecked(true);
        assertCheckedAccounts(adapter, A);

        // The row of account A is never rebound, as if it were scrolled off screen.
        adapter.bindViewHolder(holderB, B);
        assertFalse(holderB.accountRadioButton.isChecked());
        holderB.accountRadioButton.setChecked(true);
        assertCheckedAccounts(adapter, B);

        // Recycling the row of account B for account A shows it as not selected.
        adapter.bindViewHolder(holderB, A);
        assertFalse(holderB.accountRadioButton.isChecked());
        assertCheckedAccounts(adapter, B);
    }

    @Test
    public void setAccounts_clearsSelection() {
        BravePermissionAccountsListAdapter adapter = createAdapter(Mode.MULTIPLE_ACCOUNT_SELECTION);
        ViewHolder holder = adapter.onCreateViewHolder(mParent, 0);
        adapter.bindViewHolder(holder, A);
        holder.accountCheck.setChecked(true);

        adapter.setAccounts(mAccounts);
        assertCheckedAccounts(adapter);
        adapter.bindViewHolder(holder, A);
        assertFalse(holder.accountCheck.isChecked());
    }

    private BravePermissionAccountsListAdapter createAdapter(Mode mode) {
        return new BravePermissionAccountsListAdapter(mAccounts, mode, null, null);
    }

    private void assertCheckedAccounts(
            BravePermissionAccountsListAdapter adapter, int... expectedPositions) {
        AccountInfo[] expected = new AccountInfo[expectedPositions.length];
        for (int i = 0; i < expectedPositions.length; i++) {
            expected[i] = mAccounts[expectedPositions[i]];
        }
        assertArrayEquals(expected, adapter.getCheckedAccounts());
    }

    private static AccountInfo createAccount(String address) {
        AccountId accountId = new AccountId();
        accountId.coin = CoinType.ETH;
        accountId.address = address;
        accountId.uniqueKey = "unique_key_" + address;
        AccountInfo accountInfo = new AccountInfo();
        accountInfo.accountId = accountId;
        accountInfo.address = address;
        accountInfo.name = "Account " + address.charAt(2);
        return accountInfo;
    }
}
