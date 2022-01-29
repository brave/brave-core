/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class AddAccountActivity extends BraveWalletBaseActivity {
    private String mAddress;
    private String mName;
    private boolean mIsUpdate;
    private boolean mIsImported;
    private EditText mPrivateKeyControl;
    private EditText mAddAccountText;
    private static final int FILE_PICKER_REQUEST_CODE = 1;

    public AddAccountActivity() {
        mIsUpdate = false;
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_add_account);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle(getResources().getString(R.string.add_account));

        final Button btnAdd = findViewById(R.id.btn_add);
        btnAdd.setEnabled(false);

        mAddAccountText = findViewById(R.id.add_account_text);
        mAddAccountText.addTextChangedListener(new TextWatcher() {
            @Override
            public void afterTextChanged(Editable s) {}

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // Disable add button if input is empty
                String inputText = s.toString().trim();

                btnAdd.setEnabled(!TextUtils.isEmpty(inputText));
            }
        });

        mPrivateKeyControl = findViewById(R.id.import_account_text);

        EditText importAccountPasswordText = findViewById(R.id.import_account_password_text);

        btnAdd.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mKeyringService != null) {
                    if (mIsUpdate) {
                        if (mIsImported) {
                            mKeyringService.setKeyringImportedAccountName(
                                    BraveWalletConstants.DEFAULT_KEYRING_ID, mAddress,
                                    mAddAccountText.getText().toString(), result -> {
                                        if (result) {
                                            Intent returnIntent = new Intent();
                                            returnIntent.putExtra(Utils.NAME,
                                                    mAddAccountText.getText().toString());
                                            setResult(Activity.RESULT_OK, returnIntent);
                                            finish();
                                        } else {
                                            mAddAccountText.setError(
                                                    getString(R.string.account_update_failed));
                                        }
                                    });
                        } else {
                            mKeyringService.setKeyringDerivedAccountName(
                                    BraveWalletConstants.DEFAULT_KEYRING_ID, mAddress,
                                    mAddAccountText.getText().toString(), result -> {
                                        if (result) {
                                            Intent returnIntent = new Intent();
                                            returnIntent.putExtra(Utils.NAME,
                                                    mAddAccountText.getText().toString());
                                            setResult(Activity.RESULT_OK, returnIntent);
                                            finish();
                                        } else {
                                            mAddAccountText.setError(
                                                    getString(R.string.account_update_failed));
                                        }
                                    });
                        }
                    } else if (!TextUtils.isEmpty(mPrivateKeyControl.getText().toString())) {
                        if (Utils.isJSONValid(mPrivateKeyControl.getText().toString())) {
                            mKeyringService.importAccountFromJson(
                                    mAddAccountText.getText().toString(),
                                    importAccountPasswordText.getText().toString(),
                                    mPrivateKeyControl.getText().toString(), (result, address) -> {
                                        if (result) {
                                            setResult(Activity.RESULT_OK);
                                            Utils.clearClipboard(
                                                    mPrivateKeyControl.getText().toString(), 0);
                                            Utils.clearClipboard(
                                                    importAccountPasswordText.getText().toString(),
                                                    0);
                                            finish();
                                        } else {
                                            mAddAccountText.setError(getString(
                                                    R.string.wallet_failed_to_import_account));
                                        }
                                    });
                        } else {
                            mKeyringService.importAccount(mAddAccountText.getText().toString(),
                                    mPrivateKeyControl.getText().toString().trim(),
                                    (result, address) -> {
                                        if (result) {
                                            setResult(Activity.RESULT_OK);
                                            Utils.clearClipboard(
                                                    mPrivateKeyControl.getText().toString(), 0);
                                            finish();
                                        } else {
                                            mAddAccountText.setError(getString(
                                                    R.string.wallet_failed_to_import_account));
                                        }
                                    });
                        }
                    } else {
                        mKeyringService.addAccount(
                                mAddAccountText.getText().toString(), CoinType.ETH, result -> {
                                    if (result) {
                                        setResult(Activity.RESULT_OK);
                                        finish();
                                    } else {
                                        mAddAccountText.setError(
                                                getString(R.string.account_name_empty_error));
                                    }
                                });
                    }
                }
            }
        });

        TextView importBtn = findViewById(R.id.import_btn);
        importBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent chooseFile = new Intent(Intent.ACTION_GET_CONTENT);
                chooseFile.setType("*/*");
                chooseFile = Intent.createChooser(
                        chooseFile, getResources().getString(R.string.choose_a_file));
                startActivityForResult(chooseFile, FILE_PICKER_REQUEST_CODE);
            }
        });

        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        if (getIntent() != null) {
            mAddress = getIntent().getStringExtra(Utils.ADDRESS);
            mName = getIntent().getStringExtra(Utils.NAME);
            mIsImported = getIntent().getBooleanExtra(Utils.ISIMPORTED, false);
            mIsUpdate = getIntent().getBooleanExtra(Utils.ISUPDATEACCOUNT, false);
        }
        if (mIsUpdate) {
            Button btnAdd = findViewById(R.id.btn_add);
            btnAdd.setText(getResources().getString(R.string.update));
            mAddAccountText.setText(mName);
            getSupportActionBar().setTitle(getResources().getString(R.string.update_account));
            findViewById(R.id.import_account_layout).setVisibility(View.GONE);
            findViewById(R.id.import_account_title).setVisibility(View.GONE);
        }
        if (mIsUpdate) {
            return;
        }

        getWindow().setFlags(
                WindowManager.LayoutParams.FLAG_SECURE, WindowManager.LayoutParams.FLAG_SECURE);
        assert mKeyringService != null;
        mKeyringService.getKeyringInfo(BraveWalletConstants.DEFAULT_KEYRING_ID, keyringInfo -> {
            if (keyringInfo != null) {
                mAddAccountText.setText(getUniqueNextAccountName(keyringInfo.accountInfos, 1));
            }
        });
    }

    private String getUniqueNextAccountName(AccountInfo[] accountInfos, int number) {
        String accountName = getString(
                R.string.new_account_prefix, String.valueOf(accountInfos.length + number));
        for (AccountInfo accountInfo : accountInfos) {
            if (accountInfo.name.equals(accountName)) {
                return getUniqueNextAccountName(accountInfos, number + 1);
            }
        }

        return accountName;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case FILE_PICKER_REQUEST_CODE:
                if (resultCode == -1) {
                    try {
                        Uri fileUri = data.getData();
                        InputStream inputStream = getContentResolver().openInputStream(fileUri);
                        BufferedReader br = new BufferedReader(new InputStreamReader(inputStream));
                        StringBuilder sb = new StringBuilder();
                        String line;
                        while ((line = br.readLine()) != null) {
                            sb.append(line + "\n");
                        }
                        String content = sb.toString();
                        if (Utils.isJSONValid(content)) {
                            findViewById(R.id.import_account_password_layout)
                                    .setVisibility(View.VISIBLE);
                        }
                        mPrivateKeyControl.setText(content);
                    } catch (Exception ex) {
                        Log.e("NTP", ex.getMessage());
                    }
                }
                break;
        }

        super.onActivityResult(requestCode, resultCode, data);
    }
}
