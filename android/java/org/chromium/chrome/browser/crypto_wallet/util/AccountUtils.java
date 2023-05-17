/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import org.chromium.brave_wallet.mojom.AccountInfo;

import java.util.List;
import java.util.function.Predicate;

public class AccountUtils {
    public static AccountInfo findAccount(
            List<AccountInfo> accountInfos, Predicate<AccountInfo> predicate) {
        if (JavaUtils.anyNull(accountInfos, predicate) || accountInfos.isEmpty()) return null;
        return accountInfos.stream()
                .filter(accountInfo -> predicate.test(accountInfo))
                .findFirst()
                .orElse(null);
    }
}
