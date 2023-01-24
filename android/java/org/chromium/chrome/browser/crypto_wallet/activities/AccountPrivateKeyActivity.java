/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.content.Context;
import android.hardware.biometrics.BiometricPrompt;
import android.os.Build;
import android.os.CancellationSignal;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.method.HideReturnsTransformationMethod;
import android.text.method.PasswordTransformationMethod;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.ContextCompat;

import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.util.KeystoreHelper;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Executor;

public class AccountPrivateKeyActivity extends BraveWalletBaseActivity {
    private String mAddress;
    private String mPasswordFromBiometric;
    private int mCoinType;
    private boolean mIsPrivateKeyShown;
    private boolean mIsPasswordEntered;
    private TextView mWalletTitle;
    private EditText mWalletPassword;
    private EditText mPrivateKeyText;
    private ImageView mBiometricWalletImage;
    private Button mShowPrivateKeyBtn;

    @Override
    protected void triggerLayoutInflation() {
        getWindow().setFlags(
                WindowManager.LayoutParams.FLAG_SECURE, WindowManager.LayoutParams.FLAG_SECURE);
        setContentView(R.layout.activity_account_private_key);

        if (getIntent() != null) {
            mAddress = getIntent().getStringExtra(Utils.ADDRESS);
            mCoinType = getIntent().getIntExtra(Utils.COIN_TYPE, CoinType.ETH);
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setTitle(getResources().getString(R.string.private_key));

        mWalletTitle = findViewById(R.id.tv_wallet_password_title);
        mWalletPassword = findViewById(R.id.et_wallet_password);
        mBiometricWalletImage = findViewById(R.id.iv_biometric_unlock_wallet);
        mPrivateKeyText = findViewById(R.id.private_key_text);

        TextView copyToClipboardText = findViewById(R.id.copy_to_clipboard_text);
        copyToClipboardText.setOnClickListener(v
                -> Utils.saveTextToClipboard(AccountPrivateKeyActivity.this,
                        mPrivateKeyText.getText().toString(), R.string.text_has_been_copied, true));

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
        mPasswordFromBiometric = "";
        mWalletPassword.addTextChangedListener(new FilterTextWatcherPassword());

        mShowPrivateKeyBtn = findViewById(R.id.btn_show_private_key);
        mShowPrivateKeyBtn.setOnClickListener(v -> {
            if (mIsPasswordEntered) {
                if (!mIsPrivateKeyShown) {
                    mPrivateKeyText.setTransformationMethod(
                            HideReturnsTransformationMethod.getInstance());
                    mShowPrivateKeyBtn.setText(getResources().getString(R.string.hide_private_key));
                } else {
                    mPrivateKeyText.setTransformationMethod(
                            PasswordTransformationMethod.getInstance());
                    mShowPrivateKeyBtn.setText(getResources().getString(R.string.show_private_key));
                }
                mIsPrivateKeyShown = !mIsPrivateKeyShown;
            } else {
                String passwordToUse = mPasswordFromBiometric.isEmpty()
                        ? mWalletPassword.getText().toString()
                        : mPasswordFromBiometric;
                mKeyringService.encodePrivateKeyForExport(
                        mAddress, passwordToUse, mCoinType, (privateKey) -> {
                            if (privateKey.isEmpty()) {
                                showPasswordRelatedControls(true);
                                mWalletPassword.setError(
                                        getString(R.string.incorrect_password_error));

                                return;
                            }
                            mIsPasswordEntered = true;
                            showPasswordRelatedControls(false);
                            findViewById(R.id.ll_private_key_layout).setVisibility(View.VISIBLE);

                            mPrivateKeyText.setText(privateKey);
                            mShowPrivateKeyBtn.setText(
                                    getResources().getString(R.string.show_private_key));
                        });
            }
        });
        mBiometricWalletImage.setOnClickListener(v -> {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P
                    && Utils.isBiometricAvailable(this)) {
                showPasswordRelatedControls(false);
                createBiometricPrompt();
            }
        });

        onInitialLayoutInflationComplete();

        checkOnBiometric();
    }

    private class FilterTextWatcherPassword implements TextWatcher {
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            enableDisableContinueButton();
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
    }

    private void checkOnBiometric() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P
                || !KeystoreHelper.shouldUseBiometricOnUnlock()
                || !Utils.isBiometricAvailable(this)) {
            showPasswordRelatedControls(true);

            return;
        }
        createBiometricPrompt();
    }

    private void showPasswordRelatedControls(boolean show) {
        int visibility = show ? View.VISIBLE : View.GONE;
        mWalletTitle.setVisibility(visibility);
        mWalletPassword.setVisibility(visibility);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && Utils.isBiometricAvailable(this)
                && KeystoreHelper.shouldUseBiometricOnUnlock()) {
            mBiometricWalletImage.setVisibility(visibility);
        }
    }

    private void enableDisableContinueButton() {
        if (!TextUtils.isEmpty(mWalletPassword.getText()) || !mPasswordFromBiometric.isEmpty()) {
            mShowPrivateKeyBtn.setEnabled(true);
            mShowPrivateKeyBtn.setAlpha(1.0f);
        } else {
            mShowPrivateKeyBtn.setEnabled(false);
            mShowPrivateKeyBtn.setAlpha(0.5f);
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.P)
    private void createBiometricPrompt() {
        final BiometricPrompt.AuthenticationCallback authenticationCallback =
                new BiometricPrompt.AuthenticationCallback() {
                    @Override
                    public void onAuthenticationSucceeded(
                            BiometricPrompt.AuthenticationResult result) {
                        super.onAuthenticationSucceeded(result);
                        // We authenticated using fingerprint
                        try {
                            mPasswordFromBiometric = KeystoreHelper.decryptText();
                            if (mPasswordFromBiometric.isEmpty()) {
                                showPasswordRelatedControls(true);

                                return;
                            }
                            enableDisableContinueButton();
                            mShowPrivateKeyBtn.performClick();
                        } catch (Exception exc) {
                            showPasswordRelatedControls(true);

                            return;
                        }
                    }

                    @Override
                    public void onAuthenticationError(int errorCode, CharSequence errString) {
                        super.onAuthenticationError(errorCode, errString);

                        if (!TextUtils.isEmpty(errString)) {
                            Toast.makeText(AccountPrivateKeyActivity.this, errString,
                                         Toast.LENGTH_SHORT)
                                    .show();
                        }
                        // Even though we have an error, we still let to proceed
                        showPasswordRelatedControls(true);
                    }
                };
        showFingerprintDialog(authenticationCallback);
    }

    @RequiresApi(api = Build.VERSION_CODES.P)
    private void showFingerprintDialog(
            @NonNull final BiometricPrompt.AuthenticationCallback authenticationCallback) {
        Executor executor = ContextCompat.getMainExecutor(this);
        new BiometricPrompt.Builder(this)
                .setTitle(getResources().getString(R.string.fingerprint_unlock))
                .setDescription(getResources().getString(R.string.use_fingerprint_text))
                .setNegativeButton(getResources().getString(android.R.string.cancel), executor,
                        (dialog, which)
                                -> authenticationCallback.onAuthenticationError(
                                        BiometricPrompt.BIOMETRIC_ERROR_USER_CANCELED, ""))
                .build()
                .authenticate(new CancellationSignal(), executor, authenticationCallback);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if (getCurrentFocus() != null) {
            InputMethodManager imm =
                    (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(), 0);
        }
        return super.dispatchTouchEvent(ev);
    }
}
