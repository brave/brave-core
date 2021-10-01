/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import org.chromium.brave_wallet.mojom.ErcToken;

public class WalletListItemModel {
    private int icon;
    private String title;
    private String subTitle;
    private String text1;
    private String text2;
    private ErcToken mErcToken;
    private boolean mIsImportedAccount;

    public WalletListItemModel(int icon, String title, String subTitle, String text1, String text2,
            boolean isImportedAccount) {
        this.icon = icon;
        this.title = title;
        this.subTitle = subTitle;
        this.text1 = text1;
        this.text2 = text2;
        this.mIsImportedAccount = isImportedAccount;
    }

    public WalletListItemModel(
            int icon, String title, String subTitle, String text1, String text2) {
        this.icon = icon;
        this.title = title;
        this.subTitle = subTitle;
        this.text1 = text1;
        this.text2 = text2;
    }

    public void setErcToken(ErcToken ercToken) {
        mErcToken = ercToken;
    }

    public ErcToken getErcToken() {
        return mErcToken;
    }

    public void setIsImportedAccount(boolean isImportedAccount) {
        mIsImportedAccount = isImportedAccount;
    }

    public boolean getIsImportedAccount() {
        return mIsImportedAccount;
    }

    public int getIcon() {
        return icon;
    }

    public String getTitle() {
        return title;
    }

    public String getSubTitle() {
        return subTitle;
    }

    public String getText1() {
        return text1;
    }

    public String getText2() {
        return text2;
    }
}
