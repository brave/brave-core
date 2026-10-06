/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import static org.chromium.build.NullUtil.assertNonNull;
import static org.chromium.build.NullUtil.assumeNonNull;

import android.os.Bundle;

import androidx.preference.Preference;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.supplier.ObservableSuppliers;
import org.chromium.base.supplier.SettableMonotonicObservableSupplier;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.DefaultWallet;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

@NullMarked
public class BraveWalletPreferences extends BravePreferenceFragment
        implements ConnectionErrorHandler, Preference.OnPreferenceChangeListener {
    private static final String PREF_BRAVE_WALLET_AUTOLOCK = "pref_brave_wallet_autolock";

    private static final String BRAVE_WALLET_WEB3_NOTIFICATION_SWITCH = "web3_notifications_switch";
    private static final String BRAVE_WALLET_WEB3_NFT_DISCOVERY_SWITCH =
            "nft_auto_discovery_switch";
    private static final String BRAVE_WALLET_WEB3_NFT_DISCOVERY_LEARN_MORE =
            "nft_auto_discovery_learn_more";
    // A global preference, default state is on
    public static final String PREF_BRAVE_WALLET_WEB3_NOTIFICATIONS =
            "pref_brave_wallet_web3_notifications";

    private static final String PREF_DEFAULT_ETHEREUM_WALLET = "default_ethereum_wallet";
    private static final String PREF_DEFAULT_SOLANA_WALLET = "default_solana_wallet";

    private static final String PREF_BRAVE_WALLET_RESET = "pref_brave_wallet_reset";

    private BraveDialogPreference mDefaultEthereumWallet;
    private BraveDialogPreference mDefaultSolanaWallet;
    private BraveWalletAutoLockPreferences mPrefAutolock;
    private ChromeSwitchPreference mWeb3NotificationsSwitch;
    private @Nullable ChromeSwitchPreference mWeb3NftDiscoverySwitch;

    private @Nullable KeyringService mKeyringService;
    private @Nullable BraveWalletService mBraveWalletService;

    private final SettableMonotonicObservableSupplier<String> mPageTitle =
            ObservableSuppliers.createMonotonic();

    public static boolean getPrefWeb3NotificationsEnabled() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(PREF_BRAVE_WALLET_WEB3_NOTIFICATIONS, true);
    }

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
        mPageTitle.set(getString(R.string.brave_ui_brave_wallet));
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_wallet_preferences);

        BraveWalletResetPreference braveWalletResetPreference =
                findPreference(PREF_BRAVE_WALLET_RESET);
        if (braveWalletResetPreference != null) {
            braveWalletResetPreference.setProfile(getProfile());
        }

        setUpNftDiscoveryPreference();
        mDefaultEthereumWallet = findPreference(PREF_DEFAULT_ETHEREUM_WALLET);
        if (mDefaultEthereumWallet != null) {
            mDefaultEthereumWallet.setOnPreferenceChangeListener(this);
            mDefaultEthereumWallet.setEnabled(false);
        }
        mDefaultSolanaWallet = findPreference(PREF_DEFAULT_SOLANA_WALLET);
        if (mDefaultSolanaWallet != null) {
            mDefaultSolanaWallet.setOnPreferenceChangeListener(this);
            mDefaultSolanaWallet.setEnabled(false);
        }

        mPrefAutolock = findPreference(PREF_BRAVE_WALLET_AUTOLOCK);
        mWeb3NotificationsSwitch = findPreference(BRAVE_WALLET_WEB3_NOTIFICATION_SWITCH);
        if (mWeb3NotificationsSwitch != null) {
            mWeb3NotificationsSwitch.setChecked(
                    BraveWalletPreferences.getPrefWeb3NotificationsEnabled());
            mWeb3NotificationsSwitch.setOnPreferenceChangeListener(this);
        }

        initKeyringService();
        initBraveWalletService();
    }

    @Override
    public MonotonicObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    @Override
    public String getMainMenuKey() {
        return "brave_wallet";
    }

    private void setupDefaultWalletPreference(
            final BraveDialogPreference walletPreference,
            @DefaultWallet.EnumType final Integer defaultWallet) {
        walletPreference.setEnabled(true);
        if (defaultWallet == DefaultWallet.BRAVE_WALLET_PREFER_EXTENSION) {
            walletPreference.setSummary(
                    requireActivity()
                            .getResources()
                            .getString(R.string.settings_default_wallet_option_2));
            walletPreference.setCheckedIndex(1);
        } else {
            walletPreference.setSummary(
                    requireActivity()
                            .getResources()
                            .getString(R.string.settings_default_wallet_option_1));
            walletPreference.setCheckedIndex(0);
        }
    }

    @Override
    public void onDisplayPreferenceDialog(Preference preference) {
        if (preference instanceof BraveDialogPreference) {
            BravePreferenceDialogFragment dialogFragment =
                    BravePreferenceDialogFragment.newInstance(preference);

            // `setTargetFragment()` must be called even if Lint says the method is deprecated.
            // https://issuetracker.google.com/issues/181793702
            // noinspection deprecation
            dialogFragment.setTargetFragment(this, 0);
            dialogFragment.show(getParentFragmentManager(), BravePreferenceDialogFragment.TAG);
            dialogFragment.setPreferenceDialogListener(this);
        } else {
            super.onDisplayPreferenceDialog(preference);
        }
    }

    private void setUpNftDiscoveryPreference() {
        mWeb3NftDiscoverySwitch = findPreference(BRAVE_WALLET_WEB3_NFT_DISCOVERY_SWITCH);
        assertNonNull(mWeb3NftDiscoverySwitch);
        mWeb3NftDiscoverySwitch.setEnabled(false);
        mWeb3NftDiscoverySwitch.setOnPreferenceChangeListener(this);

        BraveInlineTextButtonPreference learnMorePreference =
                findPreference(BRAVE_WALLET_WEB3_NFT_DISCOVERY_LEARN_MORE);
        if (learnMorePreference != null) {
            learnMorePreference.setTextButtonTitle(
                    getString(R.string.settings_enable_nft_discovery_desc));
            learnMorePreference.setOnPreferenceClickListener(
                    preference -> {
                        TabUtils.openUrlInCustomTab(
                                requireContext(), WalletConstants.NFT_DISCOVERY_LEARN_MORE_LINK);
                        return true;
                    });
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        refreshAutolockView();
    }

    @Override
    public void onDestroy() {
        closeServices();
        super.onDestroy();
    }

    private void closeServices() {
        if (mKeyringService != null) {
            mKeyringService.close();
            mKeyringService = null;
        }
        if (mBraveWalletService != null) {
            mBraveWalletService.close();
            mBraveWalletService = null;
        }
    }

    @Override
    public void onConnectionError(MojoException e) {
        closeServices();
        if (!canUpdatePreferences()) return;
        initKeyringService();
        initBraveWalletService();
        refreshAutolockView();
    }

    private void initBraveWalletService() {
        if (mBraveWalletService != null) return;

        // Settings can be restored before the browser activity's WalletModel is ready.
        BraveWalletService service =
                BraveWalletServiceFactory.getInstance().getBraveWalletService(this);
        mBraveWalletService = service;
        if (mDefaultEthereumWallet != null) {
            mDefaultEthereumWallet.setEnabled(false);
            service.getDefaultEthereumWallet(
                    defaultWallet -> {
                        if (!canUpdateWalletPreferences(service)) return;
                        setupDefaultWalletPreference(mDefaultEthereumWallet, defaultWallet);
                    });
        }
        if (mDefaultSolanaWallet != null) {
            mDefaultSolanaWallet.setEnabled(false);
            service.getDefaultSolanaWallet(
                    defaultWallet -> {
                        if (!canUpdateWalletPreferences(service)) return;
                        setupDefaultWalletPreference(mDefaultSolanaWallet, defaultWallet);
                    });
        }
        if (mWeb3NftDiscoverySwitch != null) {
            mWeb3NftDiscoverySwitch.setEnabled(false);
            service.getNftDiscoveryEnabled(
                    enabled -> {
                        if (!canUpdateWalletPreferences(service)) return;
                        assumeNonNull(mWeb3NftDiscoverySwitch).setChecked(enabled);
                        assumeNonNull(mWeb3NftDiscoverySwitch).setEnabled(true);
                    });
        }
    }

    private boolean canUpdateWalletPreferences(BraveWalletService service) {
        return mBraveWalletService == service && canUpdatePreferences();
    }

    private boolean canUpdatePreferences() {
        return isAdded()
                && getActivity() != null
                && !requireActivity().isFinishing()
                && !requireActivity().isDestroyed();
    }

    private void initKeyringService() {
        if (mKeyringService != null) {
            return;
        }

        mKeyringService = BraveWalletServiceFactory.getInstance().getKeyringService(this);
    }

    private void refreshAutolockView() {
        if (mKeyringService != null) {
            mKeyringService.getAutoLockMinutes(
                    minutes -> {
                        if (!canUpdatePreferences() || getView() == null) return;
                        mPrefAutolock.setSummary(
                                requireContext()
                                        .getResources()
                                        .getQuantityString(
                                                R.plurals.time_long_mins, minutes, minutes));
                        RecyclerView.ViewHolder viewHolder =
                                getListView()
                                        .findViewHolderForAdapterPosition(mPrefAutolock.getOrder());
                        if (viewHolder != null) {
                            viewHolder.itemView.invalidate();
                        }
                    });
        }
    }

    public void setPrefWeb3NotificationsEnabled(boolean enabled) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(PREF_BRAVE_WALLET_WEB3_NOTIFICATIONS, enabled);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object object) {
        String key = preference.getKey();
        if (PREF_DEFAULT_ETHEREUM_WALLET.equals(key) && mBraveWalletService != null) {
            @DefaultWallet.EnumType
            final int defaultEthereumWallet = convertToNativeDefaultWallet((Integer) object);
            mBraveWalletService.setDefaultEthereumWallet(defaultEthereumWallet);
            setupDefaultWalletPreference(mDefaultEthereumWallet, defaultEthereumWallet);
        } else if (PREF_DEFAULT_SOLANA_WALLET.equals(key) && mBraveWalletService != null) {
            @DefaultWallet.EnumType
            final int defaultSolanaWallet = convertToNativeDefaultWallet((Integer) object);
            mBraveWalletService.setDefaultSolanaWallet(defaultSolanaWallet);
            setupDefaultWalletPreference(mDefaultSolanaWallet, defaultSolanaWallet);
        } else if (BRAVE_WALLET_WEB3_NOTIFICATION_SWITCH.equals(key)) {
            setPrefWeb3NotificationsEnabled((boolean) object);
        } else if (BRAVE_WALLET_WEB3_NFT_DISCOVERY_SWITCH.equals(key)
                && mBraveWalletService != null) {
            mBraveWalletService.setNftDiscoveryEnabled((boolean) object);
        }
        return true;
    }

    @DefaultWallet.EnumType
    private int convertToNativeDefaultWallet(final Integer defaultWallet) {
        if (defaultWallet == 1) {
            return DefaultWallet.BRAVE_WALLET_PREFER_EXTENSION;
        } else {
            return DefaultWallet.NONE;
        }
    }

    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(
                    BraveWalletPreferences.class.getName(), R.xml.brave_wallet_preferences);
}
