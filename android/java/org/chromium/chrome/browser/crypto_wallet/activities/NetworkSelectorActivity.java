/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.chrome.browser.app.domain.NetworkSelectorModel.Mode.DEFAULT_WALLET_NETWORK;
import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK;
import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.appbar.MaterialToolbar;

import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.NetworkSelectorModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSelectorAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.settings.BraveSettingsLauncherImpl;
import org.chromium.chrome.browser.settings.BraveWalletAddNetworksFragment;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.components.browser_ui.settings.SettingsLauncher;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class NetworkSelectorActivity
        extends BraveWalletBaseActivity implements NetworkSelectorAdapter.NetworkClickListener {
    public static String NETWORK_SELECTOR_MODE = "network_selector_mode";
    public static String NETWORK_SELECTOR_TYPE = "network_selector_type";
    public static String NETWORK_SELECTOR_KEY = "network_selector_key";
    private static final String TAG = "NetworkSelector";
    private NetworkSelectorModel.Mode mMode;
    private NetworkSelectorModel.SelectionMode mSelectionMode;
    private RecyclerView mRVNetworkSelector;
    private NetworkSelectorAdapter mNetworkSelectorAdapter;
    private MaterialToolbar mToolbar;
    private String mSelectedNetwork;
    private SettingsLauncher mSettingsLauncher;
    private String mKey;
    private WalletModel mWalletModel;
    private NetworkSelectorModel mNetworkSelectorModel;

    /**
     * Create and return an Intent object to open network selector activity to change default wallet
     * network.
     * @param context Intent source
     * @return Intent object to open NetworkSelectorActivity in global/default wallet selection mode
     * <b>Note:</b>: It should only be called if the wallet is set up and unlocked
     */
    public static Intent createIntent(@NonNull Context context) {
        return createIntent(context, DEFAULT_WALLET_NETWORK, null);
    }

    /**
     * Create and return an Intent object to open network selector activity with key as an
     * identifier to show the previously selected local network (if available otherwise All Networks
     * as default) on {@link NetworkSelectorActivity}.
     * @param mode Whether to open network selection for default/global network mode or
     *          in local network selection mode i.e.
     *          View <=> NetworkSelection state only with All Networks option.
     * @param key as identifier to bind local state of NetworkSelection with the view. If null then
     *         use global/default network selection mode.
     * @return Intent object to open NetworkSelectorActivity in given mode
     * <b>Note:</b>: It should only be called if the wallet is set up and unlocked
     */
    public static Intent createIntent(
            @NonNull Context context, NetworkSelectorModel.Mode mode, @NonNull String key) {
        Intent braveNetworkSelectionIntent = new Intent(context, NetworkSelectorActivity.class);
        braveNetworkSelectionIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        // Either in global or local network selection mode
        braveNetworkSelectionIntent.putExtra(NETWORK_SELECTOR_MODE, mode);
        // To bind selection between the caller and NetworkSelection Activity for local state of
        // network selection
        braveNetworkSelectionIntent.putExtra(NETWORK_SELECTOR_KEY, key);
        return braveNetworkSelectionIntent;
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_network_selector);
        mToolbar = findViewById(R.id.toolbar);
        mToolbar.setTitle(R.string.brave_wallet_select_network_title);
        mToolbar.setOnMenuItemClickListener(item -> {
            if (item.getItemId() == R.id.menu_network_selector_close) {
                finish();
            } else {
                launchAddNetwork();
            }
            return true;
        });

        Intent intent = getIntent();
        mMode = JavaUtils.safeVal(
                (NetworkSelectorModel.Mode) intent.getSerializableExtra(NETWORK_SELECTOR_MODE),
                NetworkSelectorModel.Mode.DEFAULT_WALLET_NETWORK);
        // Key acts as a binding contract between the caller and network selection activity to share
        // local network selection actions in LOCAL_NETWORK_FILTER mode
        mKey = intent.getStringExtra(NETWORK_SELECTOR_KEY);
        mRVNetworkSelector = findViewById(R.id.rv_network_activity);
        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        initState();
    }

    private void initState() {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "initState " + e);
        }

        mSettingsLauncher = new BraveSettingsLauncherImpl();
        // Provide single selection type if key is null. Ideally caller should provide the selection
        // type while creating network selection model.
        NetworkSelectorModel.SelectionMode selectionMode =
                TextUtils.isEmpty(mKey) ? NetworkSelectorModel.SelectionMode.SINGLE : null;
        mNetworkSelectorModel =
                mWalletModel.getCryptoModel().getNetworkModel().openNetworkSelectorModel(
                        mKey, mMode, selectionMode, null);
        mSelectionMode = mNetworkSelectorModel.getSelectionType();
        mNetworkSelectorAdapter =
                new NetworkSelectorAdapter(this, new ArrayList<>(), mSelectionMode);
        mRVNetworkSelector.setAdapter(mNetworkSelectorAdapter);
        mNetworkSelectorAdapter.setOnNetworkItemSelected(this);
        mNetworkSelectorModel.mNetworkListsLd.observe(this, networkLists -> {
            if (networkLists == null) return;
            mNetworkSelectorAdapter.addNetworks(networkLists);
        });
        setSelectedNetworkObserver();
    }

    private void launchAddNetwork() {
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putString(ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID, "");
        fragmentArgs.putBoolean(ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK, false);
        Intent intent = mSettingsLauncher.createSettingsActivityIntent(
                this, BraveWalletAddNetworksFragment.class.getName(), fragmentArgs);
        startActivity(intent);
    }

    private void setSelectedNetworkObserver() {
        if (NetworkSelectorModel.SelectionMode.MULTI == mSelectionMode) {
            LiveDataUtil.observeOnce(mNetworkSelectorModel.mSelectedNetworks,
                    networkInfoList -> { updateNetworkSelection(networkInfoList); });
            return;
        }
        mNetworkSelectorModel.getSelectedNetwork().observe(this, networkInfoList -> {
            updateNetworkSelection(Collections.singletonList(networkInfoList));
        });
    }

    private void updateNetworkSelection(List<NetworkInfo> networkInfoList) {
        if (!networkInfoList.isEmpty()) {
            mNetworkSelectorAdapter.setSelectedNetwork(networkInfoList);
        }
    }

    @Override
    public void onNetworkItemSelected(NetworkInfo networkInfo) {
        mNetworkSelectorModel.setNetworkWithAccountCheck(
                networkInfo, isSelected -> { updateNetworkUi(networkInfo, isSelected); });
    }

    private void updateNetworkUi(NetworkInfo networkInfo, Boolean isSelected) {
        if (!isSelected) {
            Toast.makeText(this,
                         getString(R.string.brave_wallet_network_selection_error,
                                 networkInfo.chainName),
                         Toast.LENGTH_SHORT)
                    .show();
        }
        // Add little delay for smooth selection animation
        PostTask.postDelayedTask(TaskTraits.UI_DEFAULT, () -> {
            if (!isActivityFinishingOrDestroyed()) {
                finish();
            }
        }, 200);
    }

    @Override
    public void onDestroy() {
        mNetworkSelectorModel.setSelectedNetworks(mNetworkSelectorAdapter.getSelectedNetworks());
        super.onDestroy();
    }
}
