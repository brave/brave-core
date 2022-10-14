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

import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAccountActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.fragments.CreateAccountBottomSheetFragment;
import org.chromium.chrome.browser.crypto_wallet.permission.BravePermissionAccountsListAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.AccountsPermissionsHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper.DefaultFaviconHelper;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper.FaviconImageCallback;
import org.chromium.url.GURL;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;

public class ConnectAccountFragment extends BaseDAppsFragment
        implements BravePermissionAccountsListAdapter.BravePermissionDelegate {
    private TextView mWebSite;
    private TextView mAccountsConnected;
    private TextView mbtNewAccount;
    private ImageView mFavicon;
    private AccountInfo[] mAccountInfos;
    private HashSet<AccountInfo> mAccountsWithPermissions;
    private BravePermissionAccountsListAdapter mAccountsListAdapter;
    private RecyclerView mRecyclerView;
    private AccountInfo mSelectedAccount;
    private FaviconHelper mFaviconHelper;
    private DefaultFaviconHelper mDefaultFaviconHelper;
    private WalletModel mWalletModel;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        BraveActivity activity = BraveActivity.getBraveActivity();
        if (activity != null) {
            mWalletModel = activity.getWalletModel();
        }
    }

    @SuppressLint("NotifyDataSetChanged")
    private void updateAccounts() {
        if (mSelectedAccount == null || mAccountInfos == null) return;
        AccountsPermissionsHelper accountsPermissionsHelper = new AccountsPermissionsHelper(
                getBraveWalletService(), mAccountInfos, Utils.getCurrentMojomOrigin());
        accountsPermissionsHelper.checkAccounts(() -> {
            mAccountsWithPermissions = accountsPermissionsHelper.getAccountsWithPermissions();
            mAccountsConnected.setText(
                    String.format(getResources().getString(R.string.wallet_accounts_connected),
                            mAccountsWithPermissions.size()));
            if (mAccountsListAdapter == null) {
                mAccountsListAdapter =
                        new BravePermissionAccountsListAdapter(mAccountInfos, false, this);
                mRecyclerView.setAdapter(mAccountsListAdapter);
                LinearLayoutManager layoutManager = new LinearLayoutManager(getActivity());
                mRecyclerView.setLayoutManager(layoutManager);
            } else {
                mAccountsListAdapter.setAccounts(mAccountInfos);
                mAccountsListAdapter.setAccountsWithPermissions(mAccountsWithPermissions);
                mAccountsListAdapter.setSelectedAccount(mSelectedAccount);
                mAccountsListAdapter.notifyDataSetChanged();
            }
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
                BottomSheetDialogFragment sheetDialogFragment =
                        new CreateAccountBottomSheetFragment();
                sheetDialogFragment.show(
                        getChildFragmentManager(), CreateAccountBottomSheetFragment.TAG);
            }
        });
        mRecyclerView = view.findViewById(R.id.accounts_list);
        mFavicon = view.findViewById(R.id.favicon);

        getBraveWalletService().geteTldPlusOneFromOrigin(Utils.getCurrentMojomOrigin(),
                origin -> { mWebSite.setText(Utils.geteTLD(origin.eTldPlusOne)); });
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
        assert mWalletModel != null;
        mWalletModel.getKeyringModel().mAccountAllAccountsPair.observe(
                getViewLifecycleOwner(), accountInfoListPair -> {
                    mSelectedAccount = accountInfoListPair.first;
                    List<AccountInfo> accountInfos = new ArrayList<>(accountInfoListPair.second);
                    if (mSelectedAccount != null) {
                        Utils.removeIf(
                                accountInfos, account -> account.coin != mSelectedAccount.coin);
                    }
                    mAccountInfos = accountInfos.toArray(new AccountInfo[0]);
                    updateAccounts();
                });
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
    public AccountInfo getSelectedAccount() {
        return mSelectedAccount;
    }

    @Override
    public void connectAccount(AccountInfo account) {
        getBraveWalletService().addPermission(
                account.coin, Utils.getCurrentMojomOrigin(), account.address, success -> {
                    if (!success) {
                        return;
                    }
                    if (CoinType.SOL != account.coin) {
                        getKeyringService().setSelectedAccount(
                                account.address, account.coin, setSuccess -> {});
                    }
                    updateAccounts();
                });
    }

    @Override
    public void disconnectAccount(AccountInfo account) {
        getBraveWalletService().resetPermission(
                account.coin, Utils.getCurrentMojomOrigin(), account.address, success -> {
                    if (!success) {
                        return;
                    }
                    if (!mSelectedAccount.address.equals(account.address)) {
                        updateAccounts();
                        return;
                    }

                    assert mAccountsWithPermissions != null;
                    Iterator<AccountInfo> it = mAccountsWithPermissions.iterator();
                    while (it.hasNext()) {
                        AccountInfo accountInfo = it.next();
                        if (!accountInfo.address.equals(account.address)) {
                            getKeyringService().setSelectedAccount(
                                    accountInfo.address, accountInfo.coin, setSuccess -> {});
                            break;
                        }
                    }
                    updateAccounts();
                });
    }

    @Override
    public void switchAccount(AccountInfo account) {
        getKeyringService().setSelectedAccount(account.address, account.coin, setSuccess -> {
            if (setSuccess) {
                updateAccounts();
            }
        });
    }

    private GURL getCurrentHostHttpAddress() {
        ChromeTabbedActivity activity = BraveActivity.getChromeTabbedActivity();
        if (activity != null && activity.getActivityTab() != null) {
            return activity.getActivityTab().getUrl().getOrigin();
        }
        return GURL.emptyGURL();
    }
}
