/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.KeyringModel.FilecoinNetworkType;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.util.LiveDataUtil;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Arrays;

public class AddAccountActivity extends BraveWalletBaseActivity {
    private static final String TAG = "AddAccountActivity";

    private static final int FILECOIN_MAINNET_POSITION = 0;
    private static final int FILECOIN_TESTNET_POSITION = 1;

    private @CoinType.EnumType int mCoinForNewAccount;
    private AccountInfo mEditedAccountInfo;

    private EditText mPrivateKeyControl;
    private EditText mAddAccountText;
    private EditText mImportAccountPasswordText;
    private Spinner mFilecoinNetworkSpinner;
    private static final int FILE_PICKER_REQUEST_CODE = 1;
    private WalletModel mWalletModel;
    @FilecoinNetworkType private String mSelectedFilecoinNetwork;

    @NonNull
    public static Intent createIntentToAddAccount(
            @NonNull Context context, @CoinType.EnumType int coinForNewAccount) {
        Intent intent = new Intent(context, AddAccountActivity.class);
        intent.putExtra(Utils.COIN_TYPE, coinForNewAccount);
        return intent;
    }

    @Override
    protected void onPreCreate() {
        Intent intent = getIntent();
        if (intent != null) {
            mCoinForNewAccount = intent.getIntExtra(Utils.COIN_TYPE, -1);
            mEditedAccountInfo = WalletUtils.getAccountInfoFromIntent(intent);
        }
        mSelectedFilecoinNetwork = BraveWalletConstants.FILECOIN_MAINNET;
    }

    @Override
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_add_account);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle(getResources().getString(R.string.add_account));

        mAddAccountText = findViewById(R.id.add_account_text);
        mPrivateKeyControl = findViewById(R.id.import_account_text);

        mFilecoinNetworkSpinner = findViewById(R.id.filecoin_network_spinner);

        final Button btnAdd = findViewById(R.id.btn_add);
        TextView importBtn = findViewById(R.id.import_btn);
        EditText mImportAccountPasswordText = findViewById(R.id.import_account_password_text);

        btnAdd.setEnabled(false);
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "Error during triggerLayoutInflation", e);
        }

        if (mCoinForNewAccount == CoinType.FIL) {
            setupFilecoinNetworkSpinner();
        }

        mAddAccountText.addTextChangedListener(
                new TextWatcher() {
                    @Override
                    public void afterTextChanged(Editable s) {}

                    @Override
                    public void beforeTextChanged(
                            CharSequence s, int start, int count, int after) {}

                    @Override
                    public void onTextChanged(CharSequence s, int start, int before, int count) {
                        // Disable add button if input is empty.
                        String inputText = s.toString().trim();
                        btnAdd.setEnabled(!TextUtils.isEmpty(inputText));
                    }
                });

        btnAdd.setOnClickListener(
                v -> {
                    if (mEditedAccountInfo != null) {
                        updateAccountName();
                        return;
                    }

                    if (!TextUtils.isEmpty(mPrivateKeyControl.getText().toString())) {
                        importAccount(mCoinForNewAccount);
                    } else {
                        addAccount(mCoinForNewAccount);
                    }
                });

        importBtn.setOnClickListener(
                v -> {
                    Intent chooseFile = new Intent(Intent.ACTION_GET_CONTENT);
                    chooseFile.setType("*/*");
                    chooseFile =
                            Intent.createChooser(
                                    chooseFile, getResources().getString(R.string.choose_a_file));
                    startActivityForResult(chooseFile, FILE_PICKER_REQUEST_CODE);
                });

        onInitialLayoutInflationComplete();
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();

        if (mEditedAccountInfo != null) {
            Button btnAdd = findViewById(R.id.btn_add);
            btnAdd.setText(getResources().getString(R.string.update));
            mAddAccountText.setText(mEditedAccountInfo.name);
            ActionBar actionBar = getSupportActionBar();
            if (actionBar != null) {
                actionBar.setTitle(getResources().getString(R.string.update_account));
            }
            findViewById(R.id.import_account_layout).setVisibility(View.GONE);
            findViewById(R.id.import_account_title).setVisibility(View.GONE);
            return;
        }

        getWindow()
                .setFlags(
                        WindowManager.LayoutParams.FLAG_SECURE,
                        WindowManager.LayoutParams.FLAG_SECURE);
        LiveDataUtil.observeOnce(
                mWalletModel.getKeyringModel().mAccountInfos,
                accounts -> {
                    mAddAccountText.setText(
                            WalletUtils.generateUniqueAccountName(
                                    mCoinForNewAccount, accounts.toArray(new AccountInfo[0])));
                });
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
                            sb.append(line).append("\n");
                        }
                        String content = sb.toString();
                        if (Utils.isJSONValid(content)) {
                            findViewById(R.id.import_account_password_layout)
                                    .setVisibility(View.VISIBLE);
                        }
                        mPrivateKeyControl.setText(content);
                    } catch (Exception ex) {
                        Log.e(TAG, "Error while processing the selected JSON file", ex);
                    }
                }
                break;
        }

        super.onActivityResult(requestCode, resultCode, data);
    }

    private void handleUpdateAccount(boolean result) {
        if (result) {
            Intent returnIntent = new Intent();
            returnIntent.putExtra(Utils.NAME, mAddAccountText.getText().toString());
            setResult(Activity.RESULT_OK, returnIntent);
            finish();
        } else {
            mAddAccountText.setError(getString(R.string.account_update_failed));
        }
    }

    private void updateAccountName() {
        mKeyringService.setAccountName(
                mEditedAccountInfo.accountId,
                mAddAccountText.getText().toString(),
                this::handleUpdateAccount);
    }

    private void handleImportAccount(boolean result, boolean fromJson) {
        if (result) {
            setResult(Activity.RESULT_OK);
            Utils.clearClipboard(mPrivateKeyControl.getText().toString());
            if (fromJson) {
                Utils.clearClipboard(mImportAccountPasswordText.getText().toString());
            }
            finish();
        } else {
            mAddAccountText.setError(getString(R.string.wallet_failed_to_import_account));
        }
    }

    private void importAccount(@CoinType.EnumType int coinType) {
        String accountName = mAddAccountText.getText().toString();
        String privateKey = mPrivateKeyControl.getText().toString();

        if (Utils.isJSONValid(privateKey)) {
            // Import account from JSON.
            String accountPassword = mImportAccountPasswordText.getText().toString();
            mKeyringService.importAccountFromJson(
                    accountName,
                    accountPassword,
                    privateKey,
                    (account) -> handleImportAccount(account != null, true));
        } else {
            // Import account from string.
            if (coinType == CoinType.FIL) {
                mKeyringService.importFilecoinAccount(
                        accountName,
                        privateKey.trim(),
                        mSelectedFilecoinNetwork,
                        (account) -> handleImportAccount(account != null, false));
            } else {
                mKeyringService.importAccount(
                        accountName,
                        privateKey.trim(),
                        coinType,
                        (account) -> handleImportAccount(account != null, false));
            }
        }
    }

    private void addAccount(@CoinType.EnumType int coinType) {
        mWalletModel
                .getKeyringModel()
                .addAccount(
                        coinType,
                        mSelectedFilecoinNetwork,
                        mAddAccountText.getText().toString(),
                        this::handleAddAccountResult);
    }

    private void handleAddAccountResult(boolean result) {
        if (result) {
            setResult(Activity.RESULT_OK);
            finish();
        } else {
            mAddAccountText.setError(getString(R.string.account_name_empty_error));
        }
    }

    private void setupFilecoinNetworkSpinner() {
        mFilecoinNetworkSpinner.setVisibility(View.VISIBLE);
        ArrayAdapter<String> filecoinNetworkArrayAdapter =
                new ArrayAdapter<String>(
                        this,
                        android.R.layout.simple_spinner_dropdown_item,
                        Arrays.asList(
                                getString(R.string.wallet_filecoin_mainnet),
                                getString(R.string.wallet_filecoin_testnet)));
        filecoinNetworkArrayAdapter.setDropDownViewResource(
                android.R.layout.simple_spinner_dropdown_item);
        mFilecoinNetworkSpinner.setAdapter(filecoinNetworkArrayAdapter);
        mFilecoinNetworkSpinner.setOnItemSelectedListener(
                new AdapterView.OnItemSelectedListener() {
                    @Override
                    public void onItemSelected(
                            AdapterView<?> parent, View view, int position, long id) {
                        switch (position) {
                            case FILECOIN_MAINNET_POSITION:
                                mSelectedFilecoinNetwork = BraveWalletConstants.FILECOIN_MAINNET;
                                break;
                            case FILECOIN_TESTNET_POSITION:
                                mSelectedFilecoinNetwork = BraveWalletConstants.FILECOIN_TESTNET;
                                break;
                            default:
                                throw new IllegalStateException(
                                        String.format(
                                                "No Filecoin network found for position %d.",
                                                position));
                        }
                    }

                    @Override
                    public void onNothingSelected(AdapterView<?> parent) {
                        /* Unused. */
                    }
                });
    }
}
