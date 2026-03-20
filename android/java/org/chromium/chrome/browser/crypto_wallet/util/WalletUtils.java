/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.util.TabUtils;

import java.nio.ByteBuffer;
import java.util.Arrays;

@NullMarked
public class WalletUtils {
    private static final String ACCOUNT_INFO = "accountInfo";
    private static final String TAG = "WalletUtils";

    private static String getNewAccountPrefixForCoin(@CoinType.EnumType final int coinType) {
        switch (coinType) {
            case CoinType.ETH -> {
                return "Ethereum";
            }
            case CoinType.SOL -> {
                return "Solana";
            }
            case CoinType.FIL -> {
                return "Filecoin";
            }
            case CoinType.BTC -> {
                return "Bitcoin";
            }
            default -> {
                assert false;
                return "";
            }
        }
    }

    /**
     * Generates a unique account name following the Desktop strategy: count existing accounts of
     * the same coin type and use {@code count + 1} as the account number. This ensures the
     * suggested name always increments based on total accounts, even if some were renamed.
     *
     * <p>Desktop implementation: {@code suggestNewAccountName} in {@code
     * components/brave_wallet_ui/utils/address-utils.ts}.
     */
    @SuppressWarnings("NoStreams")
    public static String generateUniqueAccountName(
            @CoinType.EnumType final int coinType, final AccountInfo[] accountInfos) {
        final Context context = ContextUtils.getApplicationContext();
        final int sameCoinAccountCount =
                (int)
                        Arrays.stream(accountInfos)
                                .filter(acc -> acc.accountId.coin == coinType)
                                .count();

        return context.getString(
                R.string.new_account_prefix,
                getNewAccountPrefixForCoin(coinType),
                String.valueOf(sameCoinAccountCount + 1));
    }

    public static boolean accountIdsEqual(AccountId left, AccountId right) {
        return left.uniqueKey.equals(right.uniqueKey);
    }

    public static boolean accountIdsEqual(@Nullable AccountInfo left, @Nullable AccountInfo right) {
        // Return false if either account is null since we can't compare null accounts
        // This is a fix to avoid https://github.com/brave/brave-browser/issues/43261
        if (left == null || right == null) {
            return false;
        }
        return accountIdsEqual(left.accountId, right.accountId);
    }

    @Nullable
    public static AccountInfo getAccountInfoFromIntent(@NonNull final Intent intent) {
        byte[] bytes = intent.getByteArrayExtra(ACCOUNT_INFO);
        if (bytes == null) {
            return null;
        }
        return AccountInfo.deserialize(ByteBuffer.wrap(bytes));
    }

    /**
     * Opens a web Wallet tab.
     *
     * @param forceNewTab when {@code true} it closes all Wallet tabs starting with {@link
     *     BraveActivity#BRAVE_WALLET_BASE_URL} before opening a new tab. Otherwise, it tries to
     *     refresh an old tab whose URL starts with @link BraveActivity#BRAVE_WALLET_BASE_URL}.
     */
    public static void openWebWallet(final boolean forceNewTab) {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            if (forceNewTab) {
                activity.closeAllTabsByOrigin(BraveActivity.BRAVE_WALLET_ORIGIN);
            }

            activity.openNewOrRefreshExistingTab(
                    BraveActivity.BRAVE_WALLET_ORIGIN, BraveActivity.BRAVE_WALLET_URL);
            TabUtils.bringChromeTabbedActivityToTheTop(activity);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "Error while opening wallet tab.", e);
        }
    }

    /** Closes all Wallet tabs that contains the base URL `brave://wallet`. */
    public static void closeWebWallet() {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            activity.closeAllTabsByOrigin(BraveActivity.BRAVE_WALLET_ORIGIN);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "Error while closing the Wallet tab.", e);
        }
    }

    public static void openWalletHelpCenter(@Nullable Context context) {
        if (context == null) return;
        TabUtils.openUrlInCustomTab(context, WalletConstants.WALLET_HELP_CENTER);
    }
}
