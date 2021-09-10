/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAccountActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.List;

public class AccountsFragment extends Fragment implements OnWalletListItemClick {
    private static final int ACCOUNT_REQUEST_CODE = 2;
    private View rootView;
    private WalletCoinAdapter walletCoinAdapter;
    public static AccountsFragment newInstance() {
        return new AccountsFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setHasOptionsMenu(true);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        rootView = inflater.inflate(R.layout.fragment_accounts, container, false);
        return rootView;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        TextView addAccountBtn = view.findViewById(R.id.add_account_btn);
        addAccountBtn.setOnClickListener(v -> {
            Intent addAccountActivityIntent = new Intent(getActivity(), AddAccountActivity.class);
            startActivityForResult(addAccountActivityIntent, ACCOUNT_REQUEST_CODE);
        });

        setUpAccountList(view);
        setUpSecondaryAccountList(view);
    }

    private void setUpAccountList(View view) {
        RecyclerView rvAccounts = view.findViewById(R.id.rv_accounts);
        walletCoinAdapter = new WalletCoinAdapter(WalletCoinAdapter.AdapterType.ACCOUNTS_LIST);
        KeyringController keyringController = getKeyringController();
        if (keyringController != null) {
            keyringController.getDefaultKeyringInfo(keyringInfo -> {
                if (keyringInfo != null) {
                    AccountInfo[] accountInfos = keyringInfo.accountInfos;
                    List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
                    for (AccountInfo accountInfo : accountInfos) {
                        walletListItemModelList.add(new WalletListItemModel(R.drawable.ic_eth,
                                accountInfo.name, accountInfo.address, null, null));
                    }
                    if (walletCoinAdapter != null) {
                        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
                        walletCoinAdapter.setOnWalletListItemClick(AccountsFragment.this);
                        walletCoinAdapter.setWalletListItemType(Utils.ACCOUNT_ITEM);
                        rvAccounts.setAdapter(walletCoinAdapter);
                        rvAccounts.setLayoutManager(new LinearLayoutManager(getActivity()));
                    }
                }
            });
        }
    }

    private void setUpSecondaryAccountList(View view) {
        RecyclerView rvSecondaryAccounts = view.findViewById(R.id.rv_secondary_accounts);
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.ACCOUNTS_LIST);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
        walletListItemModelList.add(new WalletListItemModel(
                R.drawable.ic_eth, "jamesmudgett.eth", "0xFCdF***DDee", null, null));
        walletListItemModelList.add(new WalletListItemModel(
                R.drawable.ic_eth, "allmydoge", "0xA1da***7af1", null, null));
        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletCoinAdapter.setOnWalletListItemClick(AccountsFragment.this);
        walletCoinAdapter.setWalletListItemType(Utils.ACCOUNT_ITEM);
        rvSecondaryAccounts.setAdapter(walletCoinAdapter);
        rvSecondaryAccounts.setLayoutManager(new LinearLayoutManager(getActivity()));
    }

    @Override
    public void onAccountClick(WalletListItemModel walletListItemModel) {
        Utils.openAccountDetailActivity(
                getActivity(), walletListItemModel.getTitle(), walletListItemModel.getSubTitle());
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == ACCOUNT_REQUEST_CODE) {
            if (resultCode == Activity.RESULT_OK) {
                if (rootView != null) setUpAccountList(rootView);
            }
        }
    }

    private KeyringController getKeyringController() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getKeyringController();
        }

        return null;
    }
}
