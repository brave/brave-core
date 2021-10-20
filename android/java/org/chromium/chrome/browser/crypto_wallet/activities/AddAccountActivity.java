/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.text.TextUtils;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.KeyringControllerFactory;
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

public class AddAccountActivity
        extends AsyncInitializationActivity implements ConnectionErrorHandler {
    private String mAddress;
    private String mName;
    private boolean mIsUpdate;
    private boolean mIsImported;
    private EditText mPrivateKeyControl;
    private KeyringController mKeyringController;
    private static final int FILE_PICKER_REQUEST_CODE = 1;

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_add_account);

        if (getIntent() != null) {
            mAddress = getIntent().getStringExtra(Utils.ADDRESS);
            mName = getIntent().getStringExtra(Utils.NAME);
            mIsImported = getIntent().getBooleanExtra(Utils.ISIMPORTED, false);
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle(getResources().getString(R.string.add_account));

        EditText addAccountText = findViewById(R.id.add_account_text);
        mPrivateKeyControl = findViewById(R.id.import_account_text);

        EditText importAccountPasswordText = findViewById(R.id.import_account_password_text);

        Button btnAdd = findViewById(R.id.btn_add);
        btnAdd.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mKeyringController != null) {
                    if (mIsUpdate) {
                        if (mIsImported) {
                            mKeyringController.setDefaultKeyringImportedAccountName(
                                    mAddress, addAccountText.getText().toString(), result -> {
                                        if (result) {
                                            Intent returnIntent = new Intent();
                                            returnIntent.putExtra(Utils.NAME,
                                                    addAccountText.getText().toString());
                                            setResult(Activity.RESULT_OK, returnIntent);
                                            finish();
                                        } else {
                                            addAccountText.setError(
                                                    getString(R.string.account_update_failed));
                                        }
                                    });
                        } else {
                            mKeyringController.setDefaultKeyringDerivedAccountName(
                                    mAddress, addAccountText.getText().toString(), result -> {
                                        if (result) {
                                            Intent returnIntent = new Intent();
                                            returnIntent.putExtra(Utils.NAME,
                                                    addAccountText.getText().toString());
                                            setResult(Activity.RESULT_OK, returnIntent);
                                            finish();
                                        } else {
                                            addAccountText.setError(
                                                    getString(R.string.account_update_failed));
                                        }
                                    });
                        }
                    } else if (!TextUtils.isEmpty(mPrivateKeyControl.getText().toString())) {
                        if (Utils.isJSONValid(mPrivateKeyControl.getText().toString())) {
                            mKeyringController.importAccountFromJson(
                                    addAccountText.getText().toString(),
                                    importAccountPasswordText.getText().toString(),
                                    mPrivateKeyControl.getText().toString(), (result, address) -> {
                                        if (result) {
                                            setResult(Activity.RESULT_OK);
                                            finish();
                                        } else {
                                            addAccountText.setError(
                                                    getString(R.string.account_name_empty_error));
                                        }
                                    });
                        } else {
                            mKeyringController.importAccount(addAccountText.getText().toString(),
                                    mPrivateKeyControl.getText().toString(), (result, address) -> {
                                        if (result) {
                                            setResult(Activity.RESULT_OK);
                                            finish();
                                        } else {
                                            addAccountText.setError(
                                                    getString(R.string.password_error));
                                        }
                                    });
                        }
                    } else {
                        mKeyringController.addAccount(
                                addAccountText.getText().toString(), result -> {
                                    if (result) {
                                        setResult(Activity.RESULT_OK);
                                        finish();
                                    } else {
                                        addAccountText.setError(
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

        if (!TextUtils.isEmpty(mAddress) && !TextUtils.isEmpty(mName)) {
            btnAdd.setText(getResources().getString(R.string.update));
            addAccountText.setText(mName);
            getSupportActionBar().setTitle(getResources().getString(R.string.update_account));
            findViewById(R.id.import_account_layout).setVisibility(View.GONE);
            findViewById(R.id.import_account_title).setVisibility(View.GONE);
            mIsUpdate = true;
        }

        onInitialLayoutInflationComplete();
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
