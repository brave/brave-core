/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.SpannableString;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.supplier.ObservableSupplier;
import org.chromium.base.supplier.ObservableSupplierImpl;
import org.chromium.brave_wallet.mojom.DefaultWallet;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.ChromeSwitchPreference;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.components.browser_ui.settings.TextMessagePreference;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.text.SpanApplier;

public class BraveWalletPreferences extends BravePreferenceFragment
        implements ConnectionErrorHandler, Preference.OnPreferenceChangeListener {
    private static final String TAG = "WalletPreferences";
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

    private BraveDialogPreference mDefaultEthereumWallet;
    private BraveDialogPreference mDefaultSolanaWallet;
    private BraveWalletAutoLockPreferences mPrefAutolock;
    private ChromeSwitchPreference mWeb3NotificationsSwitch;
    private ChromeSwitchPreference mWeb3NftDiscoverySwitch;

    private KeyringService mKeyringService;
    private WalletModel mWalletModel;

    private final ObservableSupplierImpl<String> mPageTitle = new ObservableSupplierImpl<>();

    public static boolean getPrefWeb3NotificationsEnabled() {
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();

        return sharedPreferences.getBoolean(PREF_BRAVE_WALLET_WEB3_NOTIFICATIONS, true);
    }

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, @Nullable String rootKey) {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "onCreatePreferences", e);
        }

        mPageTitle.set(getString(R.string.brave_ui_brave_wallet));
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_wallet_preferences);

        setUpNftDiscoveryPreference();
        mDefaultEthereumWallet = findPreference(PREF_DEFAULT_ETHEREUM_WALLET);
        if (mDefaultEthereumWallet != null) {
            mDefaultEthereumWallet.setOnPreferenceChangeListener(this);
            mDefaultEthereumWallet.setEnabled(false);
            if (mWalletModel != null) {
                mWalletModel
                        .getBraveWalletService()
                        .getDefaultEthereumWallet(
                                (@DefaultWallet.EnumType int defaultEthereumWallet) ->
                                        setupDefaultWalletPreference(
                                                mDefaultEthereumWallet, defaultEthereumWallet));
            }
        }
        mDefaultSolanaWallet = findPreference(PREF_DEFAULT_SOLANA_WALLET);
        if (mDefaultSolanaWallet != null) {
            mDefaultSolanaWallet.setOnPreferenceChangeListener(this);
            mDefaultSolanaWallet.setEnabled(false);
            if (mWalletModel != null) {
                mWalletModel
                        .getBraveWalletService()
                        .getDefaultSolanaWallet(
                                (@DefaultWallet.EnumType int defaultSolanaWallet) ->
                                        setupDefaultWalletPreference(
                                                mDefaultSolanaWallet, defaultSolanaWallet));
            }
        }

        mPrefAutolock = findPreference(PREF_BRAVE_WALLET_AUTOLOCK);
        mWeb3NotificationsSwitch = findPreference(BRAVE_WALLET_WEB3_NOTIFICATION_SWITCH);
        if (mWeb3NotificationsSwitch != null) {
            mWeb3NotificationsSwitch.setChecked(
                    BraveWalletPreferences.getPrefWeb3NotificationsEnabled());
            mWeb3NotificationsSwitch.setOnPreferenceChangeListener(this);
        }

        initKeyringService();
    }

    @Override
    public ObservableSupplier<String> getPageTitle() {
        return mPageTitle;
    }

    private void setupDefaultWalletPreference(
            @NonNull final BraveDialogPreference walletPreference,
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
    public void onDisplayPreferenceDialog(@NonNull Preference preference) {
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
        if (mWalletModel == null) return;
        mWeb3NftDiscoverySwitch = findPreference(BRAVE_WALLET_WEB3_NFT_DISCOVERY_SWITCH);
        mWalletModel
                .getCryptoModel()
                .isNftDiscoveryEnabled(
                        isNftDiscoveryEnabled ->
                                mWeb3NftDiscoverySwitch.setChecked(isNftDiscoveryEnabled));
        mWeb3NftDiscoverySwitch.setOnPreferenceChangeListener(this);

        TextMessagePreference learnMorePreference =
                findPreference(BRAVE_WALLET_WEB3_NFT_DISCOVERY_LEARN_MORE);
        if (learnMorePreference != null) {
            SpannableString learnMoreDesc =
                    SpanApplier.applySpans(
                            getString(R.string.settings_enable_nft_discovery_desc),
                            new SpanApplier.SpanInfo(
                                    "<LINK_1>",
                                    "</LINK_1>",
                                    new NoUnderlineClickableSpan(
                                            requireContext(),
                                            R.color.brave_link,
                                            result -> {
                                                TabUtils.openUrlInCustomTab(
                                                        requireContext(),
                                                        WalletConstants
                                                                .NFT_DISCOVERY_LEARN_MORE_LINK);
                                            })));
            learnMorePreference.setSummary(learnMoreDesc);
        }
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
        initKeyringService();
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
        SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = sharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_BRAVE_WALLET_WEB3_NOTIFICATIONS, enabled);
        sharedPreferencesEditor.apply();
    }

    @Override
    public boolean onPreferenceChange(@NonNull Preference preference, Object object) {
        String key = preference.getKey();
        if (PREF_DEFAULT_ETHEREUM_WALLET.equals(key) && mWalletModel != null) {
            @DefaultWallet.EnumType
            final int defaultEthereumWallet = convertToNativeDefaultWallet((Integer) object);
            mWalletModel.getBraveWalletService().setDefaultEthereumWallet(defaultEthereumWallet);
            setupDefaultWalletPreference(mDefaultEthereumWallet, defaultEthereumWallet);
        } else if (PREF_DEFAULT_SOLANA_WALLET.equals(key) && mWalletModel != null) {
            @DefaultWallet.EnumType
            final int defaultSolanaWallet = convertToNativeDefaultWallet((Integer) object);
            mWalletModel.getBraveWalletService().setDefaultSolanaWallet(defaultSolanaWallet);
            setupDefaultWalletPreference(mDefaultSolanaWallet, defaultSolanaWallet);
        } else if (BRAVE_WALLET_WEB3_NOTIFICATION_SWITCH.equals(key)) {
            setPrefWeb3NotificationsEnabled((boolean) object);
        } else if (BRAVE_WALLET_WEB3_NFT_DISCOVERY_SWITCH.equals(key) && mWalletModel != null) {
            mWalletModel.getCryptoModel().updateNftDiscovery((boolean) object);
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
}
