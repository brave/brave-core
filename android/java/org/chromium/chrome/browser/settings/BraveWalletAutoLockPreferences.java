/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.content.DialogInterface;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import androidx.appcompat.app.AlertDialog;
import androidx.preference.Preference;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.KeyboardVisibilityDelegate;

public class BraveWalletAutoLockPreferences
        extends Preference implements Preference.OnPreferenceClickListener, ConnectionErrorHandler {
    private String TAG = "BraveWalletAutoLockPreferences";

    public static final int WALLET_AUTOLOCK_DEFAULT_TIME = 5;

    private KeyringService mKeyringService;

    public BraveWalletAutoLockPreferences(Context context, AttributeSet attrs) {
        super(context, attrs);

        setOnPreferenceClickListener(this);

        initKeyringService();
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        showBraveWalletAutoLockDialog();
        return true;
    }

    @Override
    public void onDetached() {
        super.onDetached();
        if (mKeyringService != null) {
            mKeyringService.close();
        }
    }

    private void showBraveWalletAutoLockDialog() {
        LayoutInflater inflater =
                (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.brave_wallet_autolock_layout, null);
        final EditText input = (EditText) view.findViewById(R.id.brave_wallet_autolock_edittext);

        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int button) {
                if (button == AlertDialog.BUTTON_POSITIVE) {
                    String inputMinutes = input.getText().toString().trim();

                    setPrefWalletAutoLockTime(inputMinutes);
                } else {
                    dialog.dismiss();
                }
            }
        };

        AlertDialog.Builder alert =
                new AlertDialog.Builder(getContext(), R.style.ThemeOverlay_BrowserUI_AlertDialog);
        AlertDialog alertDialog =
                alert.setTitle(R.string.brave_wallet_settings_autolock_option)
                        .setView(view)
                        .setPositiveButton(R.string.brave_wallet_confirm_text, onClickListener)
                        .setNegativeButton(R.string.cancel, onClickListener)
                        .create();
        alertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        alertDialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface dialog) {
                KeyboardVisibilityDelegate.getInstance().showKeyboard(input);
            }
        });
        alertDialog.show();
        final Button okButton = alertDialog.getButton(AlertDialog.BUTTON_POSITIVE);
        okButton.setEnabled(false);

        input.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {}

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // Disable ok button if input is invalid
                String inputMinutes = s.toString().trim();
                int numMinutes = inputToInt(inputMinutes);
                boolean hasError = numMinutes > 60 * 24 * 7; // 7 days

                okButton.setEnabled(!hasError && inputMinutes.length() > 0);
            }
        });
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

    private void setPrefWalletAutoLockTime(String s) {
        int numMinutes = inputToInt(s);

        if (mKeyringService != null) {
            mKeyringService.setAutoLockMinutes(numMinutes, success -> {
                if (success) updateSummary(numMinutes);
            });
        }
    }

    private int inputToInt(String s) {
        int numMinutes = WALLET_AUTOLOCK_DEFAULT_TIME;
        try {
            numMinutes = Integer.parseInt(s);
        } catch (NumberFormatException e) {
            Log.d(TAG, "Input parsing failure.");
        }

        return numMinutes;
    }

    private void updateSummary(int autolockTime) {
        this.setSummary(getContext().getResources().getQuantityString(
                R.plurals.time_long_mins, autolockTime, autolockTime));
    }
}
