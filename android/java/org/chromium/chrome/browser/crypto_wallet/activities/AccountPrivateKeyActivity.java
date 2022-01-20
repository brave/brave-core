/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.text.method.HideReturnsTransformationMethod;
import android.text.method.PasswordTransformationMethod;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;

import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;
import java.util.List;

public class AccountPrivateKeyActivity extends BraveWalletBaseActivity {
    private String mAddress;
    private boolean mIsPrivateKeyShown;

    @Override
    protected void triggerLayoutInflation() {
        getWindow().setFlags(
                WindowManager.LayoutParams.FLAG_SECURE, WindowManager.LayoutParams.FLAG_SECURE);
        setContentView(R.layout.activity_account_private_key);

        if (getIntent() != null) {
            mAddress = getIntent().getStringExtra(Utils.ADDRESS);
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle(getResources().getString(R.string.private_key));

        EditText privateKeyText = findViewById(R.id.private_key_text);
        TextView copyToClipboardText = findViewById(R.id.copy_to_clipboard_text);
        copyToClipboardText.setOnClickListener(v
                -> Utils.saveTextToClipboard(AccountPrivateKeyActivity.this,
                        privateKeyText.getText().toString(), R.string.text_has_been_copied, true));

        LinearLayout bannerLayout = findViewById(R.id.wallet_backup_banner);
        bannerLayout.setVisibility(View.VISIBLE);

        TextView warningText = findViewById(R.id.warning_text);
        warningText.setText(getResources().getString(R.string.private_key_warning_text));

        ImageView bannerBtn = findViewById(R.id.backup_banner_close);
        bannerBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                bannerLayout.setVisibility(View.GONE);
            }
        });

        Button showPrivateKeyBtn = findViewById(R.id.btn_show_private_key);
        showPrivateKeyBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!mIsPrivateKeyShown) {
                    privateKeyText.setTransformationMethod(
                            HideReturnsTransformationMethod.getInstance());
                    showPrivateKeyBtn.setText(getResources().getString(R.string.hide_private_key));
                } else {
                    privateKeyText.setTransformationMethod(
                            PasswordTransformationMethod.getInstance());
                    showPrivateKeyBtn.setText(getResources().getString(R.string.show_private_key));
                }
                mIsPrivateKeyShown = !mIsPrivateKeyShown;
            }
        });

        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();

        assert mKeyringService != null;
        mKeyringService.getPrivateKeyForDefaultKeyringAccount(mAddress, (result, privateKey) -> {
            if (result) {
                EditText privateKeyText = findViewById(R.id.private_key_text);
                privateKeyText.setText(privateKey);
            }
        });
    }
}
