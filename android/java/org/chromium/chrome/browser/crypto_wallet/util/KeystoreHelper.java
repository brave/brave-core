/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.text.TextUtils;
import android.util.Base64;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.UnrecoverableEntryException;
import java.security.cert.CertificateException;

import javax.crypto.BadPaddingException;
import javax.crypto.Cipher;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.KeyGenerator;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;

public class KeystoreHelper {
    private static final String TRANSFORMATION = "AES/GCM/NoPadding";
    private static final String BRAVE_WALLET_ALIAS = "BRAVE_WALLET_ALIAS";
    private static final String ANDROID_KEY_STORE = "AndroidKeyStore";

    @Nullable
    public static Cipher getCipherForEncryption() {
        try {
            final Cipher cipher = Cipher.getInstance(TRANSFORMATION);
            cipher.init(Cipher.ENCRYPT_MODE, getSecretKey());
            return cipher;
        } catch (NoSuchAlgorithmException
                | NoSuchPaddingException
                | NoSuchProviderException
                | InvalidAlgorithmParameterException
                | InvalidKeyException e) {
            return null;
        }
    }

    @Nullable
    public static Cipher getCipherForDecryption() {
        try {
            KeyStore keyStore = KeyStore.getInstance(ANDROID_KEY_STORE);
            keyStore.load(null);

            String ivBase64 =
                    ChromeSharedPreferences.getInstance()
                            .readString(BravePreferenceKeys.BRAVE_BIOMETRICS_FOR_WALLET_IV, "");
            if (TextUtils.isEmpty(ivBase64)) {
                return null;
            }
            GCMParameterSpec spec =
                    new GCMParameterSpec(128, Base64.decode(ivBase64, Base64.DEFAULT));

            KeyStore.SecretKeyEntry secretKeyEntry =
                    (KeyStore.SecretKeyEntry) keyStore.getEntry(BRAVE_WALLET_ALIAS, null);
            if (secretKeyEntry == null) {
                return null;
            }
            SecretKey secretKey = secretKeyEntry.getSecretKey();
            final Cipher cipher = Cipher.getInstance(TRANSFORMATION);
            cipher.init(Cipher.DECRYPT_MODE, secretKey, spec);
            return cipher;
        } catch (InvalidAlgorithmParameterException
                | NoSuchPaddingException
                | UnrecoverableEntryException
                | CertificateException
                | NoSuchAlgorithmException
                | KeyStoreException
                | IOException
                | InvalidKeyException e) {
            return null;
        }
    }

    public static void useBiometricOnUnlock(String text, @NonNull final Cipher cipher) {
        if (!encryptText(text, cipher)) {
            return;
        }

        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_USE_BIOMETRICS_FOR_WALLET, true);
    }

    private static boolean encryptText(String text, @NonNull final Cipher cipher) {
        try {
            byte[] iv = cipher.getIV();
            byte[] encrypted = cipher.doFinal(text.getBytes(StandardCharsets.UTF_8));
            saveToSharedPref(iv, encrypted);
        } catch (Exception exc) {
            // Fallback to password only if we fail to encrypt it for biometric
            return false;
        }

        return true;
    }

    private static SecretKey getSecretKey()
            throws NoSuchAlgorithmException,
                    NoSuchProviderException,
                    InvalidAlgorithmParameterException {
        KeyGenerator keyGenerator =
                KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, ANDROID_KEY_STORE);

        keyGenerator.init(
                new KeyGenParameterSpec.Builder(
                                BRAVE_WALLET_ALIAS,
                                KeyProperties.PURPOSE_ENCRYPT | KeyProperties.PURPOSE_DECRYPT)
                        .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
                        .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
                        .build());

        return keyGenerator.generateKey();
    }

    private static void saveToSharedPref(byte[] iv, byte[] encrypted) {
        String ivBase64 = Base64.encodeToString(iv, Base64.DEFAULT);
        String encryptedBase64 = Base64.encodeToString(encrypted, Base64.DEFAULT);

        ChromeSharedPreferences.getInstance()
                .writeString(BravePreferenceKeys.BRAVE_BIOMETRICS_FOR_WALLET_IV, ivBase64);
        ChromeSharedPreferences.getInstance()
                .writeString(
                        BravePreferenceKeys.BRAVE_BIOMETRICS_FOR_WALLET_ENCRYPTED, encryptedBase64);
    }

    public static boolean shouldUseBiometricToUnlock() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_USE_BIOMETRICS_FOR_WALLET, false);
    }

    @NonNull
    public static String decryptText(@NonNull final Cipher cipher)
            throws KeyStoreException,
                    CertificateException,
                    NoSuchAlgorithmException,
                    InvalidAlgorithmParameterException,
                    BadPaddingException,
                    IOException,
                    NoSuchPaddingException,
                    UnrecoverableEntryException,
                    InvalidKeyException,
                    IllegalBlockSizeException {

        String encryptedBase64 =
                ChromeSharedPreferences.getInstance()
                        .readString(BravePreferenceKeys.BRAVE_BIOMETRICS_FOR_WALLET_ENCRYPTED, "");
        if (TextUtils.isEmpty(encryptedBase64)) {
            return "";
        }

        return new String(
                cipher.doFinal(Base64.decode(encryptedBase64, Base64.DEFAULT)),
                StandardCharsets.UTF_8);
    }

    public static void resetBiometric() {
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_USE_BIOMETRICS_FOR_WALLET);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_BIOMETRICS_FOR_WALLET_ENCRYPTED);
        ChromeSharedPreferences.getInstance()
                .removeKey(BravePreferenceKeys.BRAVE_BIOMETRICS_FOR_WALLET_IV);
    }
}
