/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_shields.mojom.FilterListAndroidHandler;
import org.chromium.brave_shields.mojom.SubscriptionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.settings.BravePreferenceFragment;
import org.chromium.chrome.browser.settings.BraveSettingsActivity;
import org.chromium.components.browser_ui.settings.FragmentSettingsLauncher;
import org.chromium.components.browser_ui.settings.SettingsLauncher;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.mojo_base.mojom.Value;

import java.util.ArrayList;
import java.util.Arrays;

public class ContentFilteringFragment extends BravePreferenceFragment
        implements FragmentSettingsLauncher, BraveContentFilteringListener, ConnectionErrorHandler {
    private RecyclerView mRecyclerView;

    private ContentFilteringAdapter mAdapter;
    private FilterListAndroidHandler mFilterListAndroidHandler;
    private ArrayList<SubscriptionInfo> mCustomFilterLists;
    private Value mFilterLists[];
    private MenuItem mEditItem;
    private MenuItem mDoneItem;
    private boolean mIsMenuLoaded;
    private boolean mIsGetSubscriptionsLoaded;

    // SettingsLauncher injected from main Settings Activity.
    private SettingsLauncher mSettingsLauncher;
    private ActivityResultLauncher<Intent> mAddCustomFilterResultLauncher;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_content_filtering, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        if (getActivity() != null) {
            getActivity().setTitle(R.string.content_filters_title);
        }
        super.onActivityCreated(savedInstanceState);
        setData();
    }
    @Override
    public void onAttach(@NonNull Context context) {
        super.onAttach(context);

        mAddCustomFilterResultLauncher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                ((BraveSettingsActivity) requireActivity()).getActivityResultRegistry(), result -> {
                    if (result.getResultCode() == Activity.RESULT_OK) {
                        getCustomFilterLists();
                    }
                });
    }

    private void setData() {
        initFilterListAndroidHandler();
        mRecyclerView = (RecyclerView) getView().findViewById(R.id.recyclerview);
        LinearLayoutManager linearLayoutManager =
                new LinearLayoutManager(getActivity(), LinearLayoutManager.VERTICAL, false);
        mRecyclerView.setLayoutManager(linearLayoutManager);
        mAdapter = new ContentFilteringAdapter(getActivity(), this);
        mRecyclerView.setAdapter(mAdapter);
        getCustomFilterLists();
        getFilterLists();
    }

    private void getCustomFilterLists() {
        if (mFilterListAndroidHandler != null) {
            mFilterListAndroidHandler.getSubscriptions(subscriptions -> {
                mCustomFilterLists = new ArrayList(Arrays.asList(subscriptions));
                mAdapter.setCustomFilterLists(mCustomFilterLists);
                mIsGetSubscriptionsLoaded = true;
                if (mIsMenuLoaded) {
                    checkForEmptyCustomFilterLists(true);
                }
            });
        }
    }

    private void getFilterLists() {
        if (mFilterListAndroidHandler != null) {
            mFilterListAndroidHandler.getFilterLists(filterLists -> {
                mFilterLists = filterLists.storage;
                mAdapter.setFilterLists(mFilterLists);
            });
        }
    }

    @Override
    public void onCustomFilterToggle(int position, boolean isEnable) {
        if (mFilterListAndroidHandler != null) {
            SubscriptionInfo customFilter = mCustomFilterLists.get(position);
            mFilterListAndroidHandler.enableSubscription(customFilter.subscriptionUrl, isEnable);
            customFilter.enabled = isEnable;
        }
    }

    @Override
    public void onAddCustomFiltering() {
        if (mCustomFilterLists.size() > 0) {
            isEditSelected(false);
        }
        Intent intent = mSettingsLauncher.createSettingsActivityIntent(
                getActivity(), AddCustomFilterListsFragment.class.getName(), null);
        mAddCustomFilterResultLauncher.launch(intent);
    }

    @Override
    public void onCustomFilterDelete(int position) {
        if (mFilterListAndroidHandler != null) {
            SubscriptionInfo customFilter = mCustomFilterLists.get(position);
            mFilterListAndroidHandler.deleteSubscription(customFilter.subscriptionUrl);
            mCustomFilterLists.remove(position);
            mAdapter.notifyItemRemoved(position + 1);
            mAdapter.notifyItemRangeChanged(position + 1, mAdapter.getItemCount());
            checkForEmptyCustomFilterLists(false);
        }
    }

    @Override
    public void onDefaultFilterToggle(String uuid, boolean isEnable) {
        if (mFilterListAndroidHandler != null) {
            mFilterListAndroidHandler.enableFilter(uuid, isEnable);
        }
    }

    private void checkForEmptyCustomFilterLists(boolean shouldEditVisible) {
        if (mCustomFilterLists.size() == 0) {
            isEditSelected(false);
            mEditItem.setVisible(false);
        } else if (shouldEditVisible) {
            mEditItem.setVisible(true);
        }
    }

    @Override
    public void setSettingsLauncher(SettingsLauncher settingsLauncher) {
        mSettingsLauncher = settingsLauncher;
    }

    @Override
    public void onConnectionError(MojoException e) {
        mFilterListAndroidHandler = null;
        initFilterListAndroidHandler();
    }

    private void initFilterListAndroidHandler() {
        if (mFilterListAndroidHandler != null) {
            return;
        }

        mFilterListAndroidHandler =
                FilterListServiceFactory.getInstance().getFilterListAndroidHandler(this);
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        MenuItem closeItem = menu.findItem(R.id.close_menu_id);
        if (closeItem != null) {
            closeItem.setVisible(false);
        }
        inflater.inflate(R.menu.menu_custom_filter_list, menu);
        mEditItem = menu.findItem(R.id.menu_id_edit);
        mDoneItem = menu.findItem(R.id.menu_id_done);
        mIsMenuLoaded = true;
        if (mIsGetSubscriptionsLoaded) {
            checkForEmptyCustomFilterLists(true);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.menu_id_edit) {
            isEditSelected(true);
            return true;
        } else if (id == R.id.menu_id_done) {
            isEditSelected(false);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void isEditSelected(boolean isEditable) {
        mDoneItem.setVisible(isEditable);
        mEditItem.setVisible(!isEditable);
        mAdapter.setEditable(isEditable);
    }

    @Override
    public void onDestroy() {
        if (mFilterListAndroidHandler != null) {
            mFilterListAndroidHandler.close();
        }
        super.onDestroy();
    }
}
