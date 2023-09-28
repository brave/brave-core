/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.permission;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.AddressUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BravePermissionAccountsListAdapter
        extends RecyclerView.Adapter<BravePermissionAccountsListAdapter.ViewHolder> {
    private Context mContext;
    private AccountInfo[] mAccountInfos;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private List<Integer> mCheckedPositions = new ArrayList<>();
    private boolean mCheckBoxStyle;
    private BravePermissionDelegate mDelegate;
    private HashSet<AccountInfo> mAccountsWithPermissions;
    private AccountInfo mSelectedAccount;

    public interface BravePermissionDelegate {
        default HashSet<AccountInfo> getAccountsWithPermissions() {
            return null;
        }
        default AccountInfo getSelectedAccount() {
            return null;
        }
        default void connectAccount(AccountInfo account){};
        default void disconnectAccount(AccountInfo account){};
        default void switchAccount(AccountInfo account){};
        default void onAccountCheckChanged(AccountInfo account, boolean isChecked){};
    }

    public BravePermissionAccountsListAdapter(
            AccountInfo[] accountInfo, boolean checkBoxStyle, BravePermissionDelegate delegate) {
        assert accountInfo != null;
        mAccountInfos = accountInfo;
        mCheckBoxStyle = checkBoxStyle;
        mDelegate = delegate;
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    @Override
    public @NonNull ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        mContext = parent.getContext();
        LayoutInflater inflater = LayoutInflater.from(mContext);
        View view = inflater.inflate(R.layout.brave_wallet_accounts_list_item, parent, false);
        if (!mCheckBoxStyle && mDelegate != null) {
            mAccountsWithPermissions = mDelegate.getAccountsWithPermissions();
            mSelectedAccount = mDelegate.getSelectedAccount();
        }

        return new ViewHolder(view);
    }

    public void setAccounts(AccountInfo[] accountInfo) {
        mAccountInfos = accountInfo;
    }

    public void setAccountsWithPermissions(HashSet<AccountInfo> accountsWithPermissions) {
        mAccountsWithPermissions = accountsWithPermissions;
    }

    public void setSelectedAccount(AccountInfo selectedAccount) {
        mSelectedAccount = selectedAccount;
        if (mAccountInfos == null || !mCheckBoxStyle) return;
        for (int i = 0; i < mAccountInfos.length; i++) {
            if (WalletUtils.accountIdsEqual(mSelectedAccount, mAccountInfos[i])) {
                mCheckedPositions.add(i);
                break;
            }
        }
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        final int arrayPosition = position;
        AccountInfo accountInfo = mAccountInfos[position];
        holder.titleText.setText(accountInfo.name);
        holder.subTitleText.setText(AddressUtils.getTruncatedAddress(accountInfo.address));
        Utils.setBlockiesBitmapResourceFromAccount(
                mExecutor, mHandler, holder.iconImg, accountInfo, true, false);

        if (mCheckBoxStyle) {
            holder.accountCheck.setVisibility(View.VISIBLE);
            if (mSelectedAccount != null && mSelectedAccount.address.equals(accountInfo.address)) {
                holder.accountCheck.setChecked(true);
            }
            holder.accountCheck.setOnCheckedChangeListener(
                    new CompoundButton.OnCheckedChangeListener() {
                        @Override
                        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                            if (isChecked) {
                                mCheckedPositions.add(arrayPosition);
                            } else {
                                mCheckedPositions.remove((Integer) arrayPosition);
                            }
                            if (mDelegate != null) {
                                mDelegate.onAccountCheckChanged(accountInfo, isChecked);
                            }
                        }
                    });
        } else {
            int connectionButtonText = R.string.fragment_connect_account_disconnect;
            boolean hasPermission = hasPermission(accountInfo.address);
            boolean isConnected = accountInfo.address.equals(mSelectedAccount.address);
            if (CoinType.SOL == mSelectedAccount.accountId.coin) {
                connectionButtonText = hasPermission ? R.string.brave_wallet_site_permissions_revoke
                                                     : R.string.brave_wallet_site_permissions_trust;
            } else {
                if (hasPermission) {
                    if (isConnected) {
                        connectionButtonText = R.string.fragment_connect_account_disconnect;
                    } else {
                        connectionButtonText = R.string.fragment_connect_account_switch;
                    }
                } else {
                    connectionButtonText = R.string.fragment_connect_account_connect;
                }
            }
            holder.accountAction.setText(holder.accountAction.getContext().getResources().getString(
                    connectionButtonText));

            holder.accountAction.setVisibility(View.VISIBLE);
            holder.accountAction.setOnClickListener(v -> {
                assert mDelegate != null;
                if (CoinType.SOL == accountInfo.accountId.coin) {
                    if (hasPermission) {
                        mDelegate.disconnectAccount(accountInfo);
                    } else {
                        mDelegate.connectAccount(accountInfo);
                    }
                } else {
                    if (hasPermission) {
                        if (isConnected) {
                            mDelegate.disconnectAccount(accountInfo);
                        } else {
                            mDelegate.switchAccount(accountInfo);
                        }
                    } else {
                        mDelegate.connectAccount(accountInfo);
                    }
                }
            });
        }
    }

    @Override
    public int getItemCount() {
        return mAccountInfos.length;
    }

    private boolean hasPermission(String address) {
        assert mAccountsWithPermissions != null;
        Iterator<AccountInfo> it = mAccountsWithPermissions.iterator();
        while (it.hasNext()) {
            if (it.next().address.equals(address)) {
                return true;
            }
        }

        return false;
    }

    public AccountInfo[] getCheckedAccounts() {
        AccountInfo[] checkedAccounts = new AccountInfo[mCheckedPositions.size()];
        for (int i = 0; i < mCheckedPositions.size(); i++) {
            checkedAccounts[i] = mAccountInfos[mCheckedPositions.get(i)];
        }

        return checkedAccounts;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public ImageView iconImg;
        public TextView titleText;
        public TextView subTitleText;
        public CheckBox accountCheck;
        public TextView accountAction;

        public ViewHolder(View itemView) {
            super(itemView);
            this.iconImg = itemView.findViewById(R.id.icon);
            this.titleText = itemView.findViewById(R.id.title);
            this.subTitleText = itemView.findViewById(R.id.sub_title);
            this.accountCheck = itemView.findViewById(R.id.account_check);
            this.accountAction = itemView.findViewById(R.id.account_action);
        }
    }
}
