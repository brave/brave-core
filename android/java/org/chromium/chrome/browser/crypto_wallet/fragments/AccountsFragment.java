/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

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

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.List;

public class AccountsFragment extends Fragment implements OnWalletListItemClick {
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
        return inflater.inflate(R.layout.fragment_accounts, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        TextView addAccountBtn = view.findViewById(R.id.add_account_btn);
        addAccountBtn.setOnClickListener(v -> Utils.openAddAccountActivity(getActivity()));

        setUpAccountList(view);
        setUpSecondaryAccountList(view);
    }

    private void setUpAccountList(View view) {
        RecyclerView rvAccounts = view.findViewById(R.id.rv_accounts);
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.ACCOUNTS_LIST);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();
        walletListItemModelList.add(new WalletListItemModel(
                R.drawable.ic_eth, "Account 1", "0xFCdF***DDee", null, null));
        walletListItemModelList.add(new WalletListItemModel(
                R.drawable.ic_eth, "Account 2", "0xA1da***7af1", null, null));
        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletCoinAdapter.setOnWalletListItemClick(AccountsFragment.this);
        walletCoinAdapter.setWalletListItemType(Utils.ACCOUNT_ITEM);
        rvAccounts.setAdapter(walletCoinAdapter);
        rvAccounts.setLayoutManager(new LinearLayoutManager(getActivity()));
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
    public void onAccountClick() {
        Utils.openAccountDetailActivity(getActivity());
    }
}
