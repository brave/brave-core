/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.model;

import org.chromium.brave_wallet.mojom.ErcToken;

public class WalletListItemModel {
    private int mIcon;
    private String mTitle;
    private String mSubTitle;
    private String mText1;
    private String mText2;
    private ErcToken mErcToken;
    private boolean mIsImportedAccount;
    private boolean mIsUserSelected;

    public WalletListItemModel(int icon, String title, String subTitle, String text1, String text2,
            boolean isImportedAccount) {
        mIcon = icon;
        mTitle = title;
        mSubTitle = subTitle;
        mText1 = text1;
        mText2 = text2;
        mIsImportedAccount = isImportedAccount;
    }

    public WalletListItemModel(
            int icon, String title, String subTitle, String text1, String text2) {
        mIcon = icon;
        mTitle = title;
        mSubTitle = subTitle;
        mText1 = text1;
        mText2 = text2;
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
        return mIcon;
    }

    public String getTitle() {
        return mTitle;
    }

    public String getSubTitle() {
        return mSubTitle;
    }

    public String getText1() {
        return mText1;
    }

    public String getText2() {
        return mText2;
    }

    public boolean getIsUserSelected() {
        return mIsUserSelected;
    }

    public void setIsUserSelected(boolean isUserSelected) {
        mIsUserSelected = isUserSelected;
    }
}
