/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static android.content.ClipDescription.MIMETYPE_TEXT_PLAIN;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletNativeWorker;
import org.chromium.ui.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;

public class Utils {
    public static final Pattern PASSWORD_PATTERN = Pattern.compile("^"
            + "(?=.*[0-9])" + // at least 1 digit
            "(?=.*[a-zA-Z])" + // any letter
            "(?=.*[$&+,:;=?@#|'<>.^*()%!-])" + // at least 1 special character
            "(?=\\S+$)" + // no white spaces
            ".{7,}" + // at least 7 characters
            "$");

    public static int ONBOARDING_ACTION = 1;
    public static int UNLOCK_WALLET_ACTION = 2;
    public static int RESTORE_WALLET_ACTION = 3;

    private static final String PREF_CRYPTO_ONBOARDING = "crypto_onboarding";

    public static List<String> getRecoveryPhraseAsList() {
        String[] recoveryPhraseArray =
                BraveWalletNativeWorker.getInstance().getRecoveryWords().split(" ");
        return new ArrayList<String>(Arrays.asList(recoveryPhraseArray));
    }

    public static String getRecoveryPhraseFromList(List<String> recoveryPhrases) {
        String recoveryPhrasesText = "";
        for (String phrase : recoveryPhrases) {
            recoveryPhrasesText = recoveryPhrasesText.concat(phrase).concat(" ");
        }
        return recoveryPhrasesText.trim();
    }

    public static void saveTextToClipboard(Context context, String textToCopy) {
        ClipboardManager clipboard =
                (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clip = ClipData.newPlainText("", textToCopy);
        clipboard.setPrimaryClip(clip);
        Toast.makeText(context, R.string.text_has_been_copied, Toast.LENGTH_SHORT).show();
    }

    public static String getTextFromClipboard(Context context) {
        ClipboardManager clipboard =
                (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
        String pasteData = "";
        if (!(clipboard.hasPrimaryClip())) {
            return pasteData;
        } else if (!(clipboard.getPrimaryClipDescription().hasMimeType(MIMETYPE_TEXT_PLAIN))) {
            return pasteData;
        } else {
            ClipData.Item item = clipboard.getPrimaryClip().getItemAt(0);
            return item.getText().toString();
        }
    }

    public static boolean shouldShowCryptoOnboarding() {
        SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();
        return mSharedPreferences.getBoolean(PREF_CRYPTO_ONBOARDING, true);
    }

    public static void disableCryptoOnboarding() {
        SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_CRYPTO_ONBOARDING, false);
        sharedPreferencesEditor.apply();
    }
}
