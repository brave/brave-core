/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.app.Activity;
import android.content.Intent;
import android.text.TextUtils;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;

import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.KeyringControllerFactory;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class AddAccountActivity
        extends AsyncInitializationActivity implements ConnectionErrorHandler {
    public enum AccountType {
        PRIMARY_ACCOUNT(0),
        SECONDARY_ACCOUNT(1);

        private int value;
        private static Map map = new HashMap<>();

        private AccountType(int value) {
            this.value = value;
        }

        static {
            for (AccountType activityType : AccountType.values()) {
                map.put(activityType.value, activityType);
            }
        }

        public static AccountType valueOf(int activityType) {
            return (AccountType) map.get(activityType);
        }

        public int getValue() {
            return value;
        }
    }

    private String mAddress;
    private String mName;
    private boolean mIsUpdate;
    private KeyringController mKeyringController;
    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_add_account);

        if (getIntent() != null) {
            mAddress = getIntent().getStringExtra("address");
            mName = getIntent().getStringExtra("name");
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle(getResources().getString(R.string.add_account));

        EditText addAccountText = findViewById(R.id.add_account_text);

        Button btnAdd = findViewById(R.id.btn_add);
        btnAdd.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mKeyringController != null) {
                    if (mIsUpdate) {
                        mKeyringController.setDefaultKeyringDerivedAccountName(
                                mAddress, addAccountText.getText().toString(), result -> {
                                    if (result) {
                                        Intent returnIntent = new Intent();
                                        returnIntent.putExtra(
                                                "name", addAccountText.getText().toString());
                                        setResult(Activity.RESULT_OK, returnIntent);
                                        finish();
                                    } else {
                                        addAccountText.setError(
                                                getString(R.string.account_update_failed));
                                    }
                                });
                    } else {
                        mKeyringController.addAccount(
                                addAccountText.getText().toString(), result -> {
                                    if (result) {
                                        Intent returnIntent = new Intent();
                                        returnIntent.putExtra("result", result);
                                        setResult(Activity.RESULT_OK, returnIntent);
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
                EditText privateKeyControl = findViewById(R.id.import_account_text);
                String privateKey = privateKeyControl.getText().toString();
                if (mKeyringController == null || privateKey.length() == 0) {
                    return;
                }
                mKeyringController.importAccount(
                        addAccountText.getText().toString(), privateKey, (result, address) -> {
                            if (result) {
                                Intent returnIntent = new Intent();
                                returnIntent.putExtra("result", result);
                                setResult(Activity.RESULT_OK, returnIntent);
                                finish();
                            } else {
                                addAccountText.setError(getString(R.string.password_error));
                            }
                        });
            }
        });

        if (!TextUtils.isEmpty(mAddress) && !TextUtils.isEmpty(mName)) {
            btnAdd.setText(getResources().getString(R.string.update));
            addAccountText.setText(mName);
            getSupportActionBar().setTitle(getResources().getString(R.string.update_account));
            findViewById(R.id.import_account_layout).setVisibility(View.GONE);
            findViewById(R.id.import_account_text).setVisibility(View.GONE);
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
}
