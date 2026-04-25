/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.permission;

import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.radiobutton.MaterialRadioButton;

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BravePermissionAccountsListAdapter
        extends RecyclerView.Adapter<BravePermissionAccountsListAdapter.ViewHolder> {

    /**
     * Mode determines different styles in the UI, and offers different interactions with the
     * available accounts.
     */
    public enum Mode {
        /**
         * Multiple accounts can be selected to be connected to a DApp. ETH accounts support
         * multiple account selection.
         *
         * @see BraveDappPermissionPromptDialog
         */
        MULTIPLE_ACCOUNT_SELECTION,
        /**
         * Only a single account can be selected to be connected to a DApp. SOL accounts support
         * single account selection.
         *
         * @see BraveDappPermissionPromptDialog
         */
        SINGLE_ACCOUNT_SELECTION,
        /**
         * Account connection mode shows for every account supported by a given DApp its relative
         * permission, giving also the ability to grant, or revoke it for that DApp.
         *
         * @see org.chromium.chrome.browser.crypto_wallet.fragments.dapps.ConnectAccountFragment
         */
        ACCOUNT_CONNECTION
    }

    private final ExecutorService mExecutor;
    private final Handler mHandler;
    private final List<Integer> mCheckedPositions = new ArrayList<>();
    private int mSelectedPosition = -1;
    @Nullable private final PermissionListener mDelegate;
    @Nullable private final AccountChangeListener mAccountChangeListener;
    @NonNull private final Mode mMode;
    @NonNull private AccountInfo[] mAccountInfoArray;
    private HashSet<AccountInfo> mAccountsWithPermissions;
    private AccountInfo mSelectedAccount;

    public interface PermissionListener {
        @NonNull
        HashSet<AccountInfo> getAccountsWithPermissions();

        @Nullable
        AccountInfo getSelectedAccount();

        void connectAccount(@NonNull final AccountInfo account);

        void disconnectAccount(@NonNull final AccountInfo account);

        void switchAccount(@NonNull final AccountInfo account);
    }

    public interface AccountChangeListener {
        void onAccountCheckChanged(@NonNull final AccountInfo account, final boolean checked);
    }

    public BravePermissionAccountsListAdapter(
            @NonNull final AccountInfo[] accountInfo,
            @NonNull final Mode mode,
            @Nullable final PermissionListener delegate,
            @Nullable final AccountChangeListener accountChangeListener) {
        mAccountInfoArray = accountInfo;
        mMode = mode;
        mDelegate = delegate;
        mAccountChangeListener = accountChangeListener;
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    public void setAccounts(@NonNull final AccountInfo[] accountInfo) {
        mAccountInfoArray = accountInfo;
        mCheckedPositions.clear();
    }

    public void setAccountsWithPermissions(HashSet<AccountInfo> accountsWithPermissions) {
        mAccountsWithPermissions = accountsWithPermissions;
    }

    public void setSelectedAccount(AccountInfo selectedAccount) {
        mSelectedAccount = selectedAccount;
        if (mMode == Mode.ACCOUNT_CONNECTION) return;
        for (int i = 0; i < mAccountInfoArray.length; i++) {
            if (WalletUtils.accountIdsEqual(mSelectedAccount, mAccountInfoArray[i])) {
                // Do not add duplicates.
                if (!mCheckedPositions.contains(i)) {
                    mCheckedPositions.add(i);
                }
                break;
            }
        }
    }

    @Override
    @NonNull
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        View view = inflater.inflate(R.layout.brave_wallet_accounts_list_item, parent, false);
        if (mMode == Mode.ACCOUNT_CONNECTION && mDelegate != null) {
            mAccountsWithPermissions = mDelegate.getAccountsWithPermissions();
            mSelectedAccount = mDelegate.getSelectedAccount();
        }

        ViewHolder viewHolder = new ViewHolder(view);
        if (mMode != Mode.ACCOUNT_CONNECTION) {
            viewHolder.itemView.setOnClickListener(
                    v -> {
                        if (viewHolder.accountRadioButton.getVisibility() == View.VISIBLE) {
                            viewHolder.accountRadioButton.setChecked(true);
                        } else if (viewHolder.accountCheck.getVisibility() == View.VISIBLE) {
                            viewHolder.accountCheck.setChecked(
                                    !viewHolder.accountCheck.isChecked());
                        }
                    });
        }
        return viewHolder;
    }

    @Override
    public void onBindViewHolder(
            @NonNull ViewHolder holder, int position, @NonNull List<Object> payloads) {
        // Skip binding function when payload matches the previous selected position.
        if (payloads.isEmpty()
                || !(payloads.get(0) instanceof Integer)
                || ((int) payloads.get(0)) != position) {
            super.onBindViewHolder(holder, position, payloads);
        }
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, final int position) {
        final AccountInfo accountInfo = mAccountInfoArray[position];
        holder.titleText.setText(accountInfo.name);
        holder.subTitleText.setText(Utils.getTruncatedAddress(accountInfo.address));
        Utils.setBlockiesBitmapResourceFromAccount(
                mExecutor, mHandler, holder.iconImg, accountInfo, true, false);

        switch (mMode) {
            case SINGLE_ACCOUNT_SELECTION -> {
                holder.accountRadioButton.setVisibility(View.VISIBLE);
                if (mSelectedAccount != null
                        && mSelectedAccount.address.equals(accountInfo.address)) {
                    holder.accountRadioButton.setChecked(true);
                } else {
                    holder.accountRadioButton.setChecked(position == mSelectedPosition);
                }

                holder.accountRadioButton.setOnCheckedChangeListener(
                        (buttonView, checked) -> setUpListener(holder, accountInfo, checked, true));
            }

            case MULTIPLE_ACCOUNT_SELECTION -> {
                holder.accountCheck.setVisibility(View.VISIBLE);
                if (mSelectedAccount != null
                        && mSelectedAccount.address.equals(accountInfo.address)) {
                    holder.accountCheck.setChecked(true);
                }
                holder.accountCheck.setOnCheckedChangeListener(
                        (buttonView, checked) ->
                                setUpListener(holder, accountInfo, checked, false));
            }

            case ACCOUNT_CONNECTION -> {
                int connectionButtonText;
                boolean hasPermission = hasPermission(accountInfo.address);
                boolean isConnected = accountInfo.address.equals(mSelectedAccount.address);
                if (CoinType.SOL == mSelectedAccount.accountId.coin) {
                    connectionButtonText =
                            hasPermission
                                    ? R.string.brave_wallet_site_permissions_revoke
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
                holder.accountAction.setText(
                        holder.accountAction
                                .getContext()
                                .getResources()
                                .getString(connectionButtonText));

                holder.accountAction.setVisibility(View.VISIBLE);
                holder.accountAction.setOnClickListener(
                        v -> {
                            if (mDelegate == null) {
                                return;
                            }
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
    }

    private void setUpListener(
            @NonNull final ViewHolder holder,
            @NonNull final AccountInfo accountInfo,
            final boolean checked,
            final boolean singleSelection) {
        if (checked) {
            mCheckedPositions.add(holder.getBindingAdapterPosition());
            if (singleSelection) {
                mSelectedPosition = holder.getBindingAdapterPosition();
                // Notify data set changed ONLY for more
                // than one item, otherwise there is not point
                if (getItemCount() > 1) {
                    // Notify data set changed inside `Handler#post()` because of
                    // https://issuetracker.google.com/issues/37136189
                    mHandler.post(
                            () -> {
                                // Pass the selected position in the payload so it will be
                                // excluded from `onBindViewHolder(ViewHolder, int)`. Excluding
                                // it from another binding will fully preserve animation selection.
                                notifyItemRangeChanged(0, getItemCount(), mSelectedPosition);
                            });
                }
            }
        } else {
            mCheckedPositions.remove((Integer) holder.getBindingAdapterPosition());
        }
        if (mAccountChangeListener != null) {
            mAccountChangeListener.onAccountCheckChanged(accountInfo, checked);
        }
    }

    @Override
    public int getItemCount() {
        return mAccountInfoArray.length;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    private boolean hasPermission(String address) {
        assert mAccountsWithPermissions != null;
        for (AccountInfo mAccountsWithPermission : mAccountsWithPermissions) {
            if (mAccountsWithPermission.address.equals(address)) {
                return true;
            }
        }

        return false;
    }

    public AccountInfo[] getCheckedAccounts() {
        AccountInfo[] checkedAccounts = new AccountInfo[mCheckedPositions.size()];
        for (int i = 0; i < mCheckedPositions.size(); i++) {
            checkedAccounts[i] = mAccountInfoArray[mCheckedPositions.get(i)];
        }

        return checkedAccounts;
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public ImageView iconImg;
        public TextView titleText;
        public TextView subTitleText;
        public CheckBox accountCheck;
        public MaterialRadioButton accountRadioButton;
        public TextView accountAction;

        public ViewHolder(View itemView) {
            super(itemView);
            this.iconImg = itemView.findViewById(R.id.icon);
            this.titleText = itemView.findViewById(R.id.title);
            this.subTitleText = itemView.findViewById(R.id.sub_title);
            this.accountCheck = itemView.findViewById(R.id.account_check);
            this.accountRadioButton = itemView.findViewById(R.id.account_radio_button);
            this.accountAction = itemView.findViewById(R.id.account_action);
        }
    }
}
