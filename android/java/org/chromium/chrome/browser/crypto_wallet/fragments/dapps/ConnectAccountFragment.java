/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments.dapps;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAccountActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.permission.BraveEthereumPermissionAccountsListAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.AccountsPermissionsHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper.DefaultFaviconHelper;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper.FaviconImageCallback;
import org.chromium.url.GURL;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Locale;

public class ConnectAccountFragment extends BaseDAppsFragment
        implements BraveEthereumPermissionAccountsListAdapter.BraveEthereumPermissionDelegate {
    private TextView mWebSite;
    private TextView mAccountsConnected;
    private TextView mbtNewAccount;
    private ImageView mFavicon;
    private AccountInfo[] mAccountInfos;
    private HashSet<AccountInfo> mAccountsWithPermissions;
    private BraveEthereumPermissionAccountsListAdapter mAccountsListAdapter;
    private RecyclerView mRecyclerView;
    private String mSelectedAccount;
    private FaviconHelper mFaviconHelper;
    private DefaultFaviconHelper mDefaultFaviconHelper;

    @SuppressLint("NotifyDataSetChanged")
    private void updateAccounts() {
        getKeyringService().getSelectedAccount(CoinType.ETH, address -> {
            mSelectedAccount = address != null ? address : "";
            getKeyringService().getKeyringInfo(
                    BraveWalletConstants.DEFAULT_KEYRING_ID, keyringInfo -> {
                        mAccountInfos = new AccountInfo[0];
                        if (keyringInfo != null) {
                            mAccountInfos = keyringInfo.accountInfos;
                        }
                        AccountsPermissionsHelper accountsPermissionsHelper =
                                new AccountsPermissionsHelper(getBraveWalletService(),
                                        mAccountInfos, Utils.getCurrentMojomOrigin());
                        accountsPermissionsHelper.checkAccounts(() -> {
                            mAccountsWithPermissions =
                                    accountsPermissionsHelper.getAccountsWithPermissions();
                            mAccountsConnected.setText(String.format(
                                    getResources().getString(R.string.wallet_accounts_connected),
                                    mAccountsWithPermissions.size()));
                            if (mAccountsListAdapter == null) {
                                mAccountsListAdapter =
                                        new BraveEthereumPermissionAccountsListAdapter(
                                                mAccountInfos, false, this);
                                mRecyclerView.setAdapter(mAccountsListAdapter);
                                LinearLayoutManager layoutManager =
                                        new LinearLayoutManager(getActivity());
                                mRecyclerView.setLayoutManager(layoutManager);
                            } else {
                                mAccountsListAdapter.setAccounts(mAccountInfos);
                                mAccountsListAdapter.setAccountsWithPermissions(
                                        mAccountsWithPermissions);
                                mAccountsListAdapter.setSelectedAccount(mSelectedAccount);
                                mAccountsListAdapter.notifyDataSetChanged();
                            }
                        });
                    });
        });
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_connect_account, container, false);
        mWebSite = view.findViewById(R.id.fragment_connect_account_website);
        mAccountsConnected = view.findViewById(R.id.fragment_connect_account_accounts_connected);
        mbtNewAccount = view.findViewById(R.id.fragment_connect_account_new_account_id);
        mbtNewAccount.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(getActivity(), AddAccountActivity.class);
                intent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
                startActivity(intent);
            }
        });
        mRecyclerView = view.findViewById(R.id.accounts_list);
        mFavicon = view.findViewById(R.id.favicon);

        mWebSite.setText(Utils.geteTLDFromGRUL(getCurrentHostHttpAddress()));
        initComponents();

        return view;
    }

    private void initComponents() {
        mFaviconHelper = new FaviconHelper();
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            GURL pageUrl = getCurrentHostHttpAddress();
            FaviconImageCallback imageCallback = (bitmap,
                    iconUrl) -> ConnectAccountFragment.this.onFaviconAvailable(pageUrl, bitmap);
            // 0 is a max bitmap size for download
            mFaviconHelper.getLocalFaviconImageForURL(
                    activity.getCurrentProfile(), pageUrl, 0, imageCallback);
        }
    }

    private void onFaviconAvailable(GURL pageUrl, Bitmap favicon) {
        if (favicon == null) {
            if (mDefaultFaviconHelper == null) mDefaultFaviconHelper = new DefaultFaviconHelper();
            favicon = mDefaultFaviconHelper.getDefaultFaviconBitmap(
                    getActivity().getResources(), pageUrl, true);
        }
        mFavicon.setImageBitmap(favicon);
        mFavicon.setVisibility(View.VISIBLE);
    }

    @Override
    public void onResume() {
        super.onResume();
        updateAccounts();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mFaviconHelper != null) mFaviconHelper.destroy();
        if (mDefaultFaviconHelper != null) mDefaultFaviconHelper.clearCache();
    }

    @Override
    public HashSet<AccountInfo> getAccountsWithPermissions() {
        return mAccountsWithPermissions;
    }

    @Override
    public String getSelectedAccount() {
        return mSelectedAccount;
    }

    @Override
    public void connectAccount(AccountInfo account) {
        getBraveWalletService().addPermission(
                CoinType.ETH, Utils.getCurrentMojomOrigin(), account.address, success -> {
                    if (!success) {
                        return;
                    }
                    getKeyringService().setSelectedAccount(
                            account.address, CoinType.ETH, setSuccess -> {
                                if (setSuccess) {
                                    updateAccounts();
                                }
                            });
                });
    }

    @Override
    public void disconnectAccount(AccountInfo account) {
        getBraveWalletService().resetPermission(
                CoinType.ETH, Utils.getCurrentMojomOrigin(), account.address, success -> {
                    if (!success) {
                        return;
                    }
                    if (!mSelectedAccount.equals(account.address)) {
                        updateAccounts();

                        return;
                    }

                    boolean updateCalled = false;
                    assert mAccountsWithPermissions != null;
                    Iterator<AccountInfo> it = mAccountsWithPermissions.iterator();
                    while (it.hasNext()) {
                        String currentAddress = it.next().address;
                        if (!currentAddress.equals(account.address)) {
                            updateCalled = true;
                            getKeyringService().setSelectedAccount(currentAddress, CoinType.ETH,
                                    setSuccess -> { updateAccounts(); });
                            break;
                        }
                    }
                    if (!updateCalled) {
                        updateAccounts();
                    }
                });
    }

    @Override
    public void switchAccount(AccountInfo account) {
        getKeyringService().setSelectedAccount(account.address, CoinType.ETH, setSuccess -> {
            if (setSuccess) {
                updateAccounts();
            }
        });
    }

    private GURL getCurrentHostHttpAddress() {
        ChromeTabbedActivity activity = BraveActivity.getChromeTabbedActivity();
        if (activity != null) {
            return activity.getActivityTab().getUrl().getOrigin();
        }
        return GURL.emptyGURL();
    }
}
