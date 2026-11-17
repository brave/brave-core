/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.permission;

import android.annotation.SuppressLint;
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

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
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

    private static final Object SELECTION_CHANGED_PAYLOAD = new Object();

    private final ExecutorService mExecutor;
    private final Handler mHandler;
    // Unique keys of the accounts the user checked. The selection is stored per account rather
    // than read from the check boxes, because RecyclerView reuses the same view holder for
    // different accounts while scrolling.
    private final Set<String> mCheckedAccountKeys = new HashSet<>();
    @Nullable private final PermissionListener mDelegate;
    @Nullable private final AccountChangeListener mAccountChangeListener;
    @NonNull private final Mode mMode;
    @NonNull private AccountInfo[] mAccountInfoArray;
    private HashSet<AccountInfo> mAccountsWithPermissions;
    private AccountInfo mSelectedAccount;

    public interface PermissionListener {
        @Nullable
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

    /**
     * Replaces the accounts shown in the list. Any previous selection is cleared, as it may refer
     * to accounts that are no longer present, and the list is refreshed.
     *
     * @param accountInfo accounts to show.
     */
    @SuppressLint("NotifyDataSetChanged")
    public void setAccounts(@NonNull final AccountInfo[] accountInfo) {
        mAccountInfoArray = accountInfo;
        mCheckedAccountKeys.clear();
        notifyDataSetChanged();
    }

    public void setAccountsWithPermissions(HashSet<AccountInfo> accountsWithPermissions) {
        mAccountsWithPermissions = accountsWithPermissions;
    }

    public void setSelectedAccount(AccountInfo selectedAccount) {
        mSelectedAccount = selectedAccount;
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
            if (mMode == Mode.SINGLE_ACCOUNT_SELECTION) {
                viewHolder.accountRadioButton.setOnCheckedChangeListener(
                        (buttonView, checked) -> onCheckedChanged(viewHolder, checked));
            } else {
                viewHolder.accountCheck.setOnCheckedChangeListener(
                        (buttonView, checked) -> onCheckedChanged(viewHolder, checked));
            }
        }
        return viewHolder;
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, final int position) {
        final AccountInfo accountInfo = mAccountInfoArray[position];
        holder.accountInfo = accountInfo;
        holder.titleText.setText(accountInfo.name);
        holder.subTitleText.setText(Utils.getTruncatedAddress(accountInfo.address));
        Utils.setBlockiesBitmapResourceFromAccount(
                mExecutor, mHandler, holder.iconImg, accountInfo, true, false);

        switch (mMode) {
            case SINGLE_ACCOUNT_SELECTION -> {
                holder.accountRadioButton.setVisibility(View.VISIBLE);
                holder.accountRadioButton.setChecked(isChecked(accountInfo));
            }

            case MULTIPLE_ACCOUNT_SELECTION -> {
                holder.accountCheck.setVisibility(View.VISIBLE);
                holder.accountCheck.setChecked(isChecked(accountInfo));
            }

            case ACCOUNT_CONNECTION -> {
                int connectionButtonText;
                final boolean hasPermission = hasPermission(accountInfo.address);
                final boolean isConnected =
                        mSelectedAccount != null
                                && accountInfo.address.equals(mSelectedAccount.address);
                if (CoinType.SOL == accountInfo.accountId.coin) {
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

    private int getCheckedPosition() {
        for (int i = 0; i < mAccountInfoArray.length; i++) {
            if (isChecked(mAccountInfoArray[i])) {
                return i;
            }
        }
        return RecyclerView.NO_POSITION;
    }

    private boolean isChecked(@NonNull final AccountInfo accountInfo) {
        return mCheckedAccountKeys.contains(accountInfo.accountId.uniqueKey);
    }

    private void onCheckedChanged(@NonNull final ViewHolder holder, final boolean checked) {
        final AccountInfo accountInfo = holder.accountInfo;
        // Nothing to do when the state already matches the selection, which is the case when
        // `onBindViewHolder` updates the view for the account it now shows.
        if (accountInfo == null || checked == isChecked(accountInfo)) {
            return;
        }
        final boolean singleSelection = mMode == Mode.SINGLE_ACCOUNT_SELECTION;
        final String accountKey = accountInfo.accountId.uniqueKey;
        if (checked) {
            if (singleSelection) {
                // Drop the previous selection and rebind its row, which may be off screen.
                final int previousPosition = getCheckedPosition();
                mCheckedAccountKeys.clear();
                if (previousPosition != RecyclerView.NO_POSITION) {
                    // A payload makes RecyclerView rebind the same view holder in place,
                    // preserving the radio button animation.
                    notifyItemChanged(previousPosition, SELECTION_CHANGED_PAYLOAD);
                }
            }
            mCheckedAccountKeys.add(accountKey);
        } else {
            mCheckedAccountKeys.remove(accountKey);
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
        final List<AccountInfo> checkedAccounts = new ArrayList<>();
        for (AccountInfo accountInfo : mAccountInfoArray) {
            if (isChecked(accountInfo)) {
                checkedAccounts.add(accountInfo);
            }
        }

        return checkedAccounts.toArray(new AccountInfo[0]);
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public ImageView iconImg;
        public TextView titleText;
        public TextView subTitleText;
        public CheckBox accountCheck;
        public MaterialRadioButton accountRadioButton;
        public TextView accountAction;
        // Account currently shown by this view holder, set on every bind.
        @Nullable public AccountInfo accountInfo;

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
