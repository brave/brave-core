/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.TextMessagePreference;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.text.SpanApplier;

public class BraveWalletPreferences
        extends BravePreferenceFragment implements ConnectionErrorHandler {
    private static final String TAG = "WalletPreferences";
    private static final String PREF_BRAVE_WALLET_AUTOLOCK = "pref_brave_wallet_autolock";
    private static final String PREF_BRAVE_WALLET_RESET = "pref_brave_wallet_reset";
    private static final String BRAVE_WALLET_WEB3_NOTIFICATION_SWITCH = "web3_notifications_switch";
    private static final String BRAVE_WALLET_WEB3_NFT_DISCOVERY_SWITCH =
            "nft_auto_discovery_switch";
    private static final String BRAVE_WALLET_WEB3_NFT_DISCOVERY_LEARN_MORE =
            "nft_auto_discovery_learn_more";
    // A global preference, default state is on
    public static final String PREF_BRAVE_WALLET_WEB3_NOTIFICATIONS =
            "pref_brave_wallet_web3_notifications";

    private BraveWalletAutoLockPreferences mPrefAutolock;
    private KeyringService mKeyringService;
    private ChromeSwitchPreference mWeb3NotificationsSwitch;
    private ChromeSwitchPreference mWeb3NftDiscoverySwitch;

    private WalletModel mWalletModel;

    public static boolean getPrefWeb3NotificationsEnabled() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();

        return sharedPreferences.getBoolean(PREF_BRAVE_WALLET_WEB3_NOTIFICATIONS, true);
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            setUpNftDiscoveryPreference();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreate ", e);
        }

        mPrefAutolock = (BraveWalletAutoLockPreferences) findPreference(PREF_BRAVE_WALLET_AUTOLOCK);
        mWeb3NotificationsSwitch =
                (ChromeSwitchPreference) findPreference(BRAVE_WALLET_WEB3_NOTIFICATION_SWITCH);
        mWeb3NotificationsSwitch.setChecked(
                BraveWalletPreferences.getPrefWeb3NotificationsEnabled());
        mWeb3NotificationsSwitch.setOnPreferenceChangeListener(
                (Preference preference, Object newValue) -> {
                    setPrefWeb3NotificationsEnabled((boolean) newValue);

                    return true;
                });

        InitKeyringService();
    }

    private void setUpNftDiscoveryPreference() {
        if (mWalletModel == null) return;
        mWeb3NftDiscoverySwitch =
                (ChromeSwitchPreference) findPreference(BRAVE_WALLET_WEB3_NFT_DISCOVERY_SWITCH);
        mWalletModel.getCryptoModel().isNftDiscoveryEnabled(isNftDiscoveryEnabled -> {
            mWeb3NftDiscoverySwitch.setChecked(isNftDiscoveryEnabled);
        });
        mWeb3NftDiscoverySwitch.setOnPreferenceChangeListener(
                (Preference preference, Object newValue) -> {
                    mWalletModel.getCryptoModel().updateNftDiscovery((boolean) newValue);
                    return true;
                });

        TextMessagePreference learnMorePreference =
                findPreference(BRAVE_WALLET_WEB3_NFT_DISCOVERY_LEARN_MORE);
        var learnMoreDesc =
                SpanApplier.applySpans(getString(R.string.settings_enable_nft_discovery_desc),
                        new SpanApplier.SpanInfo("<LINK_1>", "</LINK_1>",
                                new NoUnderlineClickableSpan(
                                        requireContext(), R.color.brave_link, result -> {
                                            TabUtils.openUrlInCustomTab(requireContext(),
                                                    WalletConstants.NFT_DISCOVERY_LEARN_MORE_LINK);
                                        })));
        learnMorePreference.setSummary(learnMoreDesc);
    }

    @Override
    public void onResume() {
        super.onResume();
        refreshAutolockView();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        if (mKeyringService != null) {
            mKeyringService.close();
        }
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringService.close();
        mKeyringService = null;
        InitKeyringService();
    }

    private void InitKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = KeyringServiceFactory.getInstance().getKeyringService(this);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        getActivity().setTitle(R.string.brave_ui_brave_wallet);
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_wallet_preferences);
    }

    private void refreshAutolockView() {
        if (mKeyringService != null) {
            mKeyringService.getAutoLockMinutes(minutes -> {
                mPrefAutolock.setSummary(getContext().getResources().getQuantityString(
                        R.plurals.time_long_mins, minutes, minutes));
                RecyclerView.ViewHolder viewHolder =
                        (RecyclerView.ViewHolder) getListView().findViewHolderForAdapterPosition(
                                mPrefAutolock.getOrder());
                if (viewHolder != null) {
                    viewHolder.itemView.invalidate();
                }
            });
        }
    }

    public void setPrefWeb3NotificationsEnabled(boolean enabled) {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_WALLET_WEB3_NOTIFICATIONS, enabled);
        sharedPreferencesEditor.apply();
    }
}
