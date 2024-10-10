/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK;
import static org.chromium.chrome.browser.crypto_wallet.util.WalletConstants.ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
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
import org.chromium.chrome.browser.app.domain.NetworkModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSelectorAdapter;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.settings.BraveSettingsLauncherImpl;
import org.chromium.chrome.browser.settings.BraveWalletAddNetworksFragment;
import org.chromium.components.browser_ui.settings.SettingsNavigation;

import java.util.List;
import java.util.function.Predicate;
import java.util.stream.Collectors;

public class NetworkSelectorActivity extends BraveWalletBaseActivity
        implements NetworkSelectorAdapter.NetworkClickListener {
    private static final String TAG = "NetworkSelector";
    private RecyclerView mRVNetworkSelector;
    private NetworkSelectorAdapter mNetworkSelectorAdapter;
    private MaterialToolbar mToolbar;
    private SettingsNavigation mSettingsLauncher;
    private WalletModel mWalletModel;
    private NetworkModel mNetworkModel;

    /**
     * Creates an Intent object to open network selector activity.
     *
     * @return Intent object to open network selector activity.
     *     <p><b>Note:</b>: It should only be called if the wallet is set up and unlocked.
     */
    @NonNull
    public static Intent createIntent(@NonNull final Context context) {
        Intent braveNetworkSelectionIntent = new Intent(context, NetworkSelectorActivity.class);
        braveNetworkSelectionIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        return braveNetworkSelectionIntent;
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_network_selector);
        mToolbar = findViewById(R.id.toolbar);
        mToolbar.setTitle(R.string.brave_wallet_select_network_title);
        mToolbar.setOnMenuItemClickListener(
                item -> {
                    if (item.getItemId() == R.id.menu_network_selector_close) {
                        finish();
                    } else {
                        launchAddNetwork();
                    }
                    return true;
                });
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
            Log.e(TAG, "initState", e);
            return;
        }

        mSettingsLauncher = new BraveSettingsLauncherImpl();
        mNetworkModel = mWalletModel.getCryptoModel().getNetworkModel();
        NetworkModel.NetworkLists networkLists = mNetworkModel.mNetworkLists.getValue();
        NetworkInfo selectedNetwork =
                mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.getValue();
        // See GitHub issue https://github.com/brave/brave-browser/issues/37399.
        // When live data will be refactored this check can be removed.
        if (networkLists == null || selectedNetwork == null) {
            Log.w(TAG, "Network lists and selected network must not be null.");
            assert false;
            return;
        }

        mNetworkSelectorAdapter =
                new NetworkSelectorAdapter(
                        this,
                        filterSupportedDapp(networkLists),
                        mWalletModel.getCryptoModel().getNetworkModel().mDefaultNetwork.getValue(),
                        this);
        mRVNetworkSelector.setAdapter(mNetworkSelectorAdapter);
    }

    @NonNull
    private NetworkModel.NetworkLists filterSupportedDapp(
            @NonNull final NetworkModel.NetworkLists networkLists) {
        final Predicate<NetworkInfo> supportedNetworkFilter =
                networkInfo ->
                        WalletConstants.SUPPORTED_COIN_TYPES_ON_DAPPS.contains(networkInfo.coin);

        final List<NetworkInfo> filteredPrimaryNetworkList =
                networkLists.mPrimaryNetworkList.stream()
                        .filter(supportedNetworkFilter)
                        .collect(Collectors.toList());
        final List<NetworkInfo> filteredSecondaryNetworkList =
                networkLists.mSecondaryNetworkList.stream()
                        .filter(supportedNetworkFilter)
                        .collect(Collectors.toList());
        final List<NetworkInfo> filteredTestNetworkList =
                networkLists.mTestNetworkList.stream()
                        .filter(supportedNetworkFilter)
                        .collect(Collectors.toList());
        return new NetworkModel.NetworkLists(
                filteredPrimaryNetworkList, filteredSecondaryNetworkList, filteredTestNetworkList);
    }

    private void launchAddNetwork() {
        Bundle fragmentArgs = new Bundle();
        fragmentArgs.putString(ADD_NETWORK_FRAGMENT_ARG_CHAIN_ID, "");
        fragmentArgs.putBoolean(ADD_NETWORK_FRAGMENT_ARG_ACTIVE_NETWORK, false);
        Intent intent =
                mSettingsLauncher.createSettingsIntent(
                        this, BraveWalletAddNetworksFragment.class, fragmentArgs);
        startActivity(intent);
    }

    @Override
    public void onNetworkItemSelected(@NonNull final NetworkInfo networkInfo) {
        mNetworkModel.setNetworkWithAccountCheck(
                networkInfo, false, isSelected -> updateNetworkUi(networkInfo, isSelected));
    }

    private void updateNetworkUi(NetworkInfo networkInfo, boolean isSelected) {
        if (!isSelected) {
            Toast.makeText(
                            this,
                            getString(
                                    R.string.brave_wallet_network_selection_error,
                                    networkInfo.chainName),
                            Toast.LENGTH_SHORT)
                    .show();
        }
        // Add little delay for smooth selection animation
        PostTask.postDelayedTask(
                TaskTraits.UI_DEFAULT,
                () -> {
                    if (!isActivityFinishingOrDestroyed()) {
                        finish();
                    }
                },
                200);
    }
}
