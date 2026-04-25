/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.adapters;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.adapters.AccountSelectorRecyclerView.ViewHolder;
import org.chromium.chrome.browser.crypto_wallet.listeners.AccountSelectorItemListener;
import org.chromium.chrome.browser.crypto_wallet.model.AccountSelectorItemModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.ui.base.BraveClipboardHelper;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AccountSelectorRecyclerView extends RecyclerView.Adapter<ViewHolder> {
    private Context mContext;
    private List<AccountSelectorItemModel> mAccountSelectorItemModelList = new ArrayList<>();
    private AccountSelectorItemListener mAccountSelectorItemListener;
    private final ExecutorService mExecutor;
    private final Handler mHandler;
    private int mPreviousSelectedIndex;

    public AccountSelectorRecyclerView() {
        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());
    }

    @Override
    public @NonNull ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        mContext = parent.getContext();
        LayoutInflater inflater = LayoutInflater.from(mContext);
        View accountSelectorViewHolder =
                inflater.inflate(R.layout.view_holder_account_selector, parent, false);
        return new ViewHolder(accountSelectorViewHolder);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        AccountSelectorItemModel accountSelectorItemModel =
                mAccountSelectorItemModelList.get(position);
        // When ViewHolder is re-used, it has the observers which are fired when
        // we modifying checkbox. This may cause unwanted modifying of the model.
        holder.resetObservers();

        holder.mTitleText.setText(accountSelectorItemModel.getTitle());
        holder.mSubTitleText.setText(
                Utils.stripAccountAddress(accountSelectorItemModel.getSubTitle()));
        if (accountSelectorItemModel.getText1() != null) {
            holder.mText1Text.setVisibility(View.VISIBLE);
            holder.mText1Text.setText(accountSelectorItemModel.getText1());
        }

        if (accountSelectorItemModel.getText2() != null) {
            holder.mText2Text.setVisibility(View.VISIBLE);
            holder.mText2Text.setText(accountSelectorItemModel.getText2());
        }

        holder.itemView.setOnClickListener(
                v -> {
                    mAccountSelectorItemListener.onAccountClick(accountSelectorItemModel);
                    updateSelectedNetwork(holder.getLayoutPosition());
                });

        holder.mIconImg.setImageResource(android.R.color.transparent);
        if (accountSelectorItemModel.getAccountInfo() != null) {
            Utils.setBlockiesBitmapResourceFromAccount(
                    mExecutor,
                    mHandler,
                    holder.mIconImg,
                    accountSelectorItemModel.getAccountInfo(),
                    true);
        }
        holder.itemView.setOnLongClickListener(
                v -> {
                    BraveClipboardHelper.saveTextToClipboard(
                            mContext,
                            accountSelectorItemModel.getSubTitle(),
                            R.string.address_has_been_copied,
                            false);

                    return true;
                });
        holder.mSelected.setVisibility(
                accountSelectorItemModel.isSelected() ? View.VISIBLE : View.INVISIBLE);
    }

    @Override
    public int getItemCount() {
        return mAccountSelectorItemModelList.size();
    }

    public void setWalletListItemModelList(
            List<AccountSelectorItemModel> accountSelectorItemModelList) {
        mAccountSelectorItemModelList = accountSelectorItemModelList;
    }

    public void setAccountSelectorItemListener(
            AccountSelectorItemListener accountSelectorItemListener) {
        mAccountSelectorItemListener = accountSelectorItemListener;
    }

    /**
     * Update the list's selection icon to the passed account
     *
     * @param title is the account name
     * @param subTitle is the account address
     */
    public void updateSelectedNetwork(String title, String subTitle) {
        for (int i = 0; i < mAccountSelectorItemModelList.size(); i++) {
            AccountSelectorItemModel accountSelectorItemModel =
                    mAccountSelectorItemModelList.get(i);
            if (accountSelectorItemModel.getTitle().equals(title)
                    && accountSelectorItemModel.getSubTitle().equals(subTitle)) {
                updateSelectedNetwork(i);
                break;
            }
        }
    }

    private void updateSelectedNetwork(final int selectedAccountPosition) {
        mAccountSelectorItemModelList.get(mPreviousSelectedIndex).setSelected(false);
        notifyItemChanged(mPreviousSelectedIndex);

        mAccountSelectorItemModelList.get(selectedAccountPosition).setSelected(true);
        mPreviousSelectedIndex = selectedAccountPosition;
        notifyItemChanged(selectedAccountPosition);
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        private final ImageView mIconImg;
        private final TextView mTitleText;
        private final TextView mSubTitleText;
        private final TextView mText1Text;
        private final TextView mText2Text;
        private final ImageView mSelected;

        public ViewHolder(View itemView) {
            super(itemView);
            mIconImg = itemView.findViewById(R.id.icon);
            mTitleText = itemView.findViewById(R.id.title);
            mSubTitleText = itemView.findViewById(R.id.sub_title);
            mText1Text = itemView.findViewById(R.id.text1);
            mText2Text = itemView.findViewById(R.id.text2);
            mSelected = itemView.findViewById(R.id.iv_selected);
        }

        public void resetObservers() {
            itemView.setOnClickListener(null);
        }
    }

    @SuppressLint("NotifyDataSetChanged")
    public void removeItem(AccountSelectorItemModel item) {
        mAccountSelectorItemModelList.remove(item);
        notifyDataSetChanged();
    }

    @SuppressLint("NotifyDataSetChanged")
    public void addItem(AccountSelectorItemModel item) {
        mAccountSelectorItemModelList.add(item);
        notifyDataSetChanged();
    }

    @SuppressLint("NotifyDataSetChanged")
    public void clear() {
        mAccountSelectorItemModelList.clear();
        notifyDataSetChanged();
    }
}
