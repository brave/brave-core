/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.permission;

import android.content.Context;
import android.graphics.Bitmap;
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
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Blockies;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BraveEthereumPermissionAccountsListAdapter
        extends RecyclerView.Adapter<BraveEthereumPermissionAccountsListAdapter.ViewHolder> {
    private Context mContext;
    private AccountInfo[] mAccountInfo;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private List<Integer> mCheckedPositions = new ArrayList<>();
    private boolean mCheckBoxStyle;
    private BraveEthereumPermissionDelegate mDelegate;
    private HashSet<AccountInfo> mAccountsWithPermissions;
    private String mSelectedAccount;

    public interface BraveEthereumPermissionDelegate {
        default HashSet<AccountInfo> getAccountsWithPermissions() {
            return null;
        }
        default String getSelectedAccount() {
            return null;
        }
        default void connectAccount(AccountInfo account){};
        default void disconnectAccount(AccountInfo account){};
        default void switchAccount(AccountInfo account){};
        default void onAccountCheckChanged(AccountInfo account, boolean isChecked){};
    }

    public BraveEthereumPermissionAccountsListAdapter(AccountInfo[] accountInfo,
            boolean checkBoxStyle, BraveEthereumPermissionDelegate delegate) {
        assert accountInfo != null;
        mAccountInfo = accountInfo;
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
        mAccountInfo = accountInfo;
    }

    public void setAccountsWithPermissions(HashSet<AccountInfo> accountsWithPermissions) {
        mAccountsWithPermissions = accountsWithPermissions;
    }

    public void setSelectedAccount(String selectedAccount) {
        mSelectedAccount = selectedAccount;
        if (mAccountInfo == null || !mCheckBoxStyle) return;
        for (int i = 0; i < mAccountInfo.length; i++) {
            if (mSelectedAccount.equals(mAccountInfo[i].address)) {
                mCheckedPositions.add(i);
                break;
            }
        }
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        final int arrayPosition = position;
        holder.titleText.setText(mAccountInfo[arrayPosition].name);
        holder.subTitleText.setText(Utils.stripAccountAddress(mAccountInfo[arrayPosition].address));
        setBlockiesBitmapResource(holder.iconImg, mAccountInfo[arrayPosition].address);
        if (mCheckBoxStyle) {
            holder.accountCheck.setVisibility(View.VISIBLE);
            if (mSelectedAccount != null
                    && mSelectedAccount.equals(mAccountInfo[arrayPosition].address)) {
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
                                mDelegate.onAccountCheckChanged(
                                        mAccountInfo[arrayPosition], isChecked);
                            }
                        }
                    });
        } else {
            if (hasPermission(mAccountInfo[arrayPosition].address)) {
                if (mAccountInfo[arrayPosition].address.equals(mSelectedAccount)) {
                    holder.accountAction.setText(
                            holder.accountAction.getContext().getResources().getString(
                                    R.string.fragment_connect_account_disconnect));
                } else {
                    holder.accountAction.setText(
                            holder.accountAction.getContext().getResources().getString(
                                    R.string.fragment_connect_account_switch));
                }
            } else {
                holder.accountAction.setText(
                        holder.accountAction.getContext().getResources().getString(
                                R.string.fragment_connect_account_connect));
            }
            holder.accountAction.setVisibility(View.VISIBLE);
            holder.accountAction.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    assert mDelegate != null;
                    if (holder.accountAction.getText().equals(
                                holder.accountAction.getContext().getResources().getString(
                                        R.string.fragment_connect_account_disconnect))) {
                        mDelegate.disconnectAccount(mAccountInfo[arrayPosition]);
                    } else if (holder.accountAction.getText().equals(
                                       holder.accountAction.getContext().getResources().getString(
                                               R.string.fragment_connect_account_connect))) {
                        mDelegate.connectAccount(mAccountInfo[arrayPosition]);
                    } else if (holder.accountAction.getText().equals(
                                       holder.accountAction.getContext().getResources().getString(
                                               R.string.fragment_connect_account_switch))) {
                        mDelegate.switchAccount(mAccountInfo[arrayPosition]);
                    }
                }
            });
        }
    }

    @Override
    public int getItemCount() {
        return mAccountInfo.length;
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
            checkedAccounts[i] = mAccountInfo[mCheckedPositions.get(i)];
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

    private void setBlockiesBitmapResource(ImageView iconImg, String source) {
        mExecutor.execute(() -> {
            final Bitmap bitmap = Blockies.createIcon(source, true);
            mHandler.post(() -> {
                if (iconImg != null) {
                    iconImg.setImageBitmap(bitmap);
                }
            });
        });
    }
}
