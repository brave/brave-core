/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import androidx.appcompat.app.AlertDialog;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.KeyringControllerFactory;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.KeyboardVisibilityDelegate;

public class BraveWalletResetActivity
        extends AsyncInitializationActivity implements ConnectionErrorHandler {

    private String TAG = "BraveWalletResetActivity";

    private KeyringController mKeyringController;
    private AlertDialog mAlertDialog;
    private Button mOkButton;

    @Override
    protected void triggerLayoutInflation() {
        LayoutInflater inflater =
                (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.brave_wallet_reset_content, null);
        final EditText input = (EditText) view.findViewById(R.id.brave_wallet_reset_edittext);

        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int button) {
                if (button == AlertDialog.BUTTON_POSITIVE) {
                    if (mKeyringController != null) {
                        String inputText = input.getText().toString().trim();
                        if (TextUtils.equals(inputText, "Yes")) {
                            Log.w(TAG, "Reset");
                            mKeyringController.reset();
                        }
                    } else {
                        Log.w(TAG, "mKeyringController == null");
                    }
                } else {
                    dialog.dismiss();
                }
            }
        };

        AlertDialog.Builder alert =
                new AlertDialog.Builder(this, R.style.Theme_Chromium_AlertDialog);
        mAlertDialog =
                alert.setTitle(R.string.brave_wallet_reset_settings_option)
                        .setView(view)
                        .setPositiveButton(
                                R.string.brave_wallet_confirm_text, onClickListener)
                        .setNegativeButton(R.string.cancel, onClickListener)
                        .create();
        mAlertDialog.getDelegate().setHandleNativeActionModesEnabled(false);
        mAlertDialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface dialog) {
                mOkButton = ((AlertDialog) dialog).getButton(AlertDialog.BUTTON_POSITIVE);
                mOkButton.setEnabled(false);
                KeyboardVisibilityDelegate.getInstance().showKeyboard(input);
            }
        });
        mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                finish();
            }
        });

        input.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {}

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // Disable ok button if input is invalid
                String inputText = s.toString().trim();

                if (mOkButton != null) {
                    mOkButton.setEnabled(TextUtils.equals(inputText, "Yes"));
                }
            }
        });

        onInitialLayoutInflationComplete();
    }

    @Override
    public void onResume()
    {
        super.onResume();

        mAlertDialog.show();
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringController = null;
        InitKeyringController();
    }

    private void InitKeyringController() {
        if (mKeyringController != null) {
            return;
        }

        mKeyringController = KeyringControllerFactory.getInstance().getKeyringController(this);
    }

    public KeyringController getKeyringController() {
        return mKeyringController;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        InitKeyringController();
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
    }
}
