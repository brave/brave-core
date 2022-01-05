/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.view.View;

import androidx.preference.Preference;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.components.browser_ui.settings.SettingsUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

public class BraveWalletPreferences
        extends BravePreferenceFragment implements ConnectionErrorHandler {
    private static final String PREF_BRAVE_WALLET_AUTOLOCK = "pref_brave_wallet_autolock";
    private static final String PREF_BRAVE_WALLET_RESET = "pref_brave_wallet_reset";

    private BraveWalletAutoLockPreferences mPrefAutolock;
    private KeyringService mKeyringService;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mPrefAutolock = (BraveWalletAutoLockPreferences) findPreference(PREF_BRAVE_WALLET_AUTOLOCK);

        InitKeyringService();
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
}
