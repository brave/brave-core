/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.components.permissions;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

@JNINamespace("permissions")
public class BravePermissionDialogDelegate implements ConnectionErrorHandler {
    /** Text to show before lifetime options. */
    private String mLifetimeOptionsText;

    /** Lifetime options to show to the user. Can be null if no options should be shown. */
    private String[] mLifetimeOptions;

    /** Lifetime option index selected by the user. */
    private int mSelectedLifetimeOption;

    private boolean mUseWalletLayout;
    private KeyringService mKeyringService;
    private String mConnectWalletTitle;
    private String mConnectWalletSubTitle;
    private String mConnectWalletAccountsTitle;
    private String mWalletWarningTitle;
    private String mConnectButtonText;
    private String mBackButtonText;
    private BraveAccountsListAdapter mAccountsListAdapter;

    public BravePermissionDialogDelegate() {
        mSelectedLifetimeOption = -1;
        mKeyringService = null;
        mUseWalletLayout = false;
    }

    @CalledByNative
    public void setLifetimeOptionsText(String lifetimeOptionsText) {
        mLifetimeOptionsText = lifetimeOptionsText;
    }

    public String getLifetimeOptionsText() {
        return mLifetimeOptionsText;
    }

    @CalledByNative
    public void setLifetimeOptions(String[] lifetimeOptions) {
        mLifetimeOptions = lifetimeOptions;
    }

    public String[] getLifetimeOptions() {
        return mLifetimeOptions;
    }

    public void setSelectedLifetimeOption(int idx) {
        mSelectedLifetimeOption = idx;
    }

    @CalledByNative
    public int getSelectedLifetimeOption() {
        return mSelectedLifetimeOption;
    }

    public boolean getUseWalletLayout() {
        return mUseWalletLayout;
    }

    @CalledByNative
    public void setUseWalletLayout(boolean useWalletLayout) {
        mUseWalletLayout = useWalletLayout;
        if (mUseWalletLayout) {
            InitKeyringService();
        }
    }

    @CalledByNative
    public void setWalletConnectTitle(String title) {
        mConnectWalletTitle = title;
    }

    public String getWalletConnectTitle() {
        return mConnectWalletTitle;
    }

    @CalledByNative
    public void setWalletConnectSubTitle(String subTitle) {
        mConnectWalletSubTitle = subTitle;
    }

    public String getWalletConnectSubTitle() {
        return mConnectWalletSubTitle;
    }

    @CalledByNative
    public void setWalletConnectAccountsTitle(String subTitle) {
        mConnectWalletAccountsTitle = subTitle;
    }

    public String getWalletConnectAccountsTitle() {
        return mConnectWalletAccountsTitle;
    }

    @CalledByNative
    public void setWalletWarningTitle(String warningTitle) {
        mWalletWarningTitle = warningTitle;
    }

    public String getWalletWarningTitle() {
        return mWalletWarningTitle;
    }

    @CalledByNative
    public void setConnectButtonText(String connectButtonText) {
        mConnectButtonText = connectButtonText;
    }

    public String getConnectButtonText() {
        return mConnectButtonText;
    }

    @CalledByNative
    public void setBackButtonText(String backButtonText) {
        mBackButtonText = backButtonText;
    }

    public String getBackButtonText() {
        return mBackButtonText;
    }

    @CalledByNative
    public String[] getSelectedAccounts() {
        assert mAccountsListAdapter != null;
        AccountInfo[] accountInfo = mAccountsListAdapter.getCheckedAccounts();
        String[] accounts = new String[accountInfo.length];
        for (int i = 0; i < accountInfo.length; i++) {
            accounts[i] = accountInfo[i].address;
        }

        return accounts;
    }

    @CalledByNative
    public void disconnectMojoServices() {
        if (mKeyringService == null) {
            return;
        }
        mKeyringService.close();
        mKeyringService = null;
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringService.close();
        mKeyringService = null;
        InitKeyringService();
    }

    protected void InitKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = KeyringServiceFactory.getInstance().getKeyringService(this);
    }

    public KeyringService getKeyringService() {
        return mKeyringService;
    }

    public BraveAccountsListAdapter getAccountsListAdapter(AccountInfo[] accountInfo) {
        assert mAccountsListAdapter == null;
        mAccountsListAdapter = new BraveAccountsListAdapter(accountInfo);

        return mAccountsListAdapter;
    }
}
