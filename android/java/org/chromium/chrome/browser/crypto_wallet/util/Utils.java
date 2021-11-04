/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static android.content.ClipDescription.MIMETYPE_TEXT_PLAIN;

import android.Manifest;
import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Handler;
import android.view.inputmethod.InputMethodManager;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.core.content.ContextCompat;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.Predicate;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.ErcToken;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.AccountDetailActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.AddAccountActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.AssetDetailActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Blockies;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.widget.Toast;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.NumberFormatException;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.math.MathContext;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
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

    public static int ACCOUNT_ITEM = 1;
    public static int ASSET_ITEM = 2;
    public static int TRANSACTION_ITEM = 3;

    public static final int ACCOUNT_REQUEST_CODE = 2;

    private static final String PREF_CRYPTO_ONBOARDING = "crypto_onboarding";
    public static final String ADDRESS = "address";
    public static final String NAME = "name";
    public static final String ISIMPORTED = "isImported";
    public static final String SWAP_EXCHANGE_PROXY = "0xdef1c0ded9bec7f1a1670819833240f027b25eff";

    public static List<String> getRecoveryPhraseAsList(String recoveryPhrase) {
        String[] recoveryPhraseArray = recoveryPhrase.split(" ");
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

    public static void LaunchBackupFilePicker(
            AsyncInitializationActivity activity, WindowAndroid.IntentCallback callback) {
        // Check if READ_EXTERNAL_STORAGE permission is granted.
        String storagePermission = Manifest.permission.READ_EXTERNAL_STORAGE;
        WindowAndroid window = activity.getWindowAndroid();
        assert window != null;
        if (!window.hasPermission(storagePermission)) {
            String[] requestPermissions = new String[] {storagePermission};
            window.requestPermissions(requestPermissions, (permissions, grantResults) -> {
                assert permissions.length == 1 && grantResults.length == 1;
                if (grantResults[0] == PackageManager.PERMISSION_DENIED) {
                    onFileNotSelected();
                }
            });
        }

        // Create file intent
        Intent createDocumentIntent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        createDocumentIntent.addCategory(Intent.CATEGORY_OPENABLE);
        createDocumentIntent.setType("text/*");
        createDocumentIntent.putExtra(Intent.EXTRA_TITLE, "wallet-phrase.txt");

        Intent chooser = new Intent(Intent.ACTION_CHOOSER);
        chooser.putExtra(Intent.EXTRA_INTENT, createDocumentIntent);

        if (!window.showIntent(chooser, callback, null)) {
            onFileNotSelected();
        }
    }

    public static void writeTextToFile(Uri uri, String textToSave) {
        AssetFileDescriptor assetFileDescriptor;
        Context context = ContextUtils.getApplicationContext();
        FileOutputStream outputStream;
        try {
            assetFileDescriptor = context.getContentResolver().openAssetFileDescriptor(uri, "wt");
            if (assetFileDescriptor != null) {
                outputStream = assetFileDescriptor.createOutputStream();
                if (outputStream != null) {
                    outputStream.write(textToSave.getBytes(Charset.forName("UTF-8")));
                    outputStream.close();
                }
            }
        } catch (FileNotFoundException e) {
            Log.e("wallet_Utils", "File URI not found", e);
        } catch (IOException e) {
            Log.e("wallet_Utils", "Problem creating output stream from URI", e);
        }
    }

    public static void onFileSaved() {
        Toast.makeText(ContextUtils.getApplicationContext(), R.string.text_has_been_saved,
                     Toast.LENGTH_SHORT)
                .show();
    }

    public static void onFileNotSelected() {
        Toast.makeText(ContextUtils.getApplicationContext(), R.string.text_file_not_saved,
                     Toast.LENGTH_SHORT)
                .show();
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

    public static void hideKeyboard(Activity activity) {
        InputMethodManager imm =
                (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(activity.getCurrentFocus().getWindowToken(), 0);
    }

    public static void openBuySendSwapActivity(
            Activity activity, BuySendSwapActivity.ActivityType activityType) {
        assert activity != null;
        Intent buySendSwapActivityIntent = new Intent(activity, BuySendSwapActivity.class);
        buySendSwapActivityIntent.putExtra("activityType", activityType.getValue());
        activity.startActivity(buySendSwapActivityIntent);
    }

    public static void openAssetDetailsActivity(Activity activity) {
        assert activity != null;
        Intent assetDetailIntent = new Intent(activity, AssetDetailActivity.class);
        activity.startActivity(assetDetailIntent);
    }

    public static void openAddAccountActivity(Activity activity) {
        assert activity != null;
        Intent addAccountActivityIntent = new Intent(activity, AddAccountActivity.class);
        activity.startActivity(addAccountActivityIntent);
    }

    public static String[] getNetworksList(Activity activity) {
        List<String> categories = new ArrayList<String>();
        categories.add(activity.getText(R.string.mainnet).toString());
        categories.add(activity.getText(R.string.rinkeby).toString());
        categories.add(activity.getText(R.string.ropsten).toString());
        categories.add(activity.getText(R.string.goerli).toString());
        categories.add(activity.getText(R.string.kovan).toString());
        categories.add(activity.getText(R.string.localhost).toString());

        return categories.toArray(new String[0]);
    }

    public static String[] getNetworksAbbrevList(Activity activity) {
        List<String> categories = new ArrayList<String>();
        categories.add(activity.getText(R.string.mainnet_short).toString());
        categories.add(activity.getText(R.string.rinkeby_short).toString());
        categories.add(activity.getText(R.string.ropsten_short).toString());
        categories.add(activity.getText(R.string.goerli_short).toString());
        categories.add(activity.getText(R.string.kovan_short).toString());
        categories.add(activity.getText(R.string.localhost).toString());

        return categories.toArray(new String[0]);
    }

    public static List<String> getSlippageToleranceList(Activity activity) {
        List<String> categories = new ArrayList<String>();
        categories.add(activity.getText(R.string.crypto_wallet_tolerance_05).toString());
        categories.add(activity.getText(R.string.crypto_wallet_tolerance_1).toString());
        categories.add(activity.getText(R.string.crypto_wallet_tolerance_15).toString());
        categories.add(activity.getText(R.string.crypto_wallet_tolerance_3).toString());
        categories.add(activity.getText(R.string.crypto_wallet_tolerance_6).toString());

        return categories;
    }

    public static CharSequence getNetworkText(Activity activity, String chain_id) {
        CharSequence strNetwork = activity.getText(R.string.mainnet);
        switch (chain_id) {
            case BraveWalletConstants.RINKEBY_CHAIN_ID:
                strNetwork = activity.getText(R.string.rinkeby);
                break;
            case BraveWalletConstants.ROPSTEN_CHAIN_ID:
                strNetwork = activity.getText(R.string.ropsten);
                break;
            case BraveWalletConstants.GOERLI_CHAIN_ID:
                strNetwork = activity.getText(R.string.goerli);
                break;
            case BraveWalletConstants.KOVAN_CHAIN_ID:
                strNetwork = activity.getText(R.string.kovan);
                break;
            case BraveWalletConstants.LOCALHOST_CHAIN_ID:
                strNetwork = activity.getText(R.string.localhost);
                break;
            case BraveWalletConstants.MAINNET_CHAIN_ID:
            default:
                strNetwork = activity.getText(R.string.mainnet);
        }

        return strNetwork;
    }

    public static CharSequence getNetworkShortText(Activity activity, String chain_id) {
        CharSequence strNetwork = activity.getText(R.string.mainnet_short);
        switch (chain_id) {
            case BraveWalletConstants.RINKEBY_CHAIN_ID:
                strNetwork = activity.getText(R.string.rinkeby_short);
                break;
            case BraveWalletConstants.ROPSTEN_CHAIN_ID:
                strNetwork = activity.getText(R.string.ropsten_short);
                break;
            case BraveWalletConstants.GOERLI_CHAIN_ID:
                strNetwork = activity.getText(R.string.goerli_short);
                break;
            case BraveWalletConstants.KOVAN_CHAIN_ID:
                strNetwork = activity.getText(R.string.kovan_short);
                break;
            case BraveWalletConstants.LOCALHOST_CHAIN_ID:
                strNetwork = activity.getText(R.string.localhost);
                break;
            case BraveWalletConstants.MAINNET_CHAIN_ID:
            default:
                strNetwork = activity.getText(R.string.mainnet_short);
        }

        return strNetwork;
    }

    public static String getNetworkConst(Activity activity, String network) {
        String networkConst = BraveWalletConstants.MAINNET_CHAIN_ID;
        if (network.equals(activity.getText(R.string.rinkeby).toString())) {
            networkConst = BraveWalletConstants.RINKEBY_CHAIN_ID;
        } else if (network.equals(activity.getText(R.string.ropsten).toString())) {
            networkConst = BraveWalletConstants.ROPSTEN_CHAIN_ID;
        } else if (network.equals(activity.getText(R.string.goerli).toString())) {
            networkConst = BraveWalletConstants.GOERLI_CHAIN_ID;
        } else if (network.equals(activity.getText(R.string.kovan).toString())) {
            networkConst = BraveWalletConstants.KOVAN_CHAIN_ID;
        } else if (network.equals(activity.getText(R.string.localhost).toString())) {
            networkConst = BraveWalletConstants.LOCALHOST_CHAIN_ID;
        }

        return networkConst;
    }

    private static String getDecimalsDepNumber(int decimals) {
        String strDecimals = "1";
        for (int i = 0; i < decimals; i++) {
            strDecimals += "0";
        }

        return strDecimals;
    }

    public static double fromHexWei(String number, int decimals) {
        if (number.equals("0x0")) {
            return 0;
        }
        if (number.startsWith("0x")) {
            number = number.substring(2);
        }
        if (number.isEmpty()) {
            return 0;
        }
        BigInteger bigNumber = new BigInteger(number, 16);
        BigInteger divider = new BigInteger(getDecimalsDepNumber(decimals));
        BigDecimal bDecimal = new BigDecimal(bigNumber);
        BigDecimal bDecimalRes = bDecimal.divide(new BigDecimal(divider), MathContext.DECIMAL32);
        String resStr = bDecimalRes.toPlainString();

        return Double.valueOf(resStr);
    }

    public static double fromHexGWeiToGWEI(String number) {
        try {
            if (number.equals("0x0")) {
                return 0;
            }
            if (number.startsWith("0x")) {
                number = number.substring(2);
            }
            if (number.isEmpty()) {
                return 0;
            }
            BigInteger bigNumber = new BigInteger(number, 16);
            String resStr = bigNumber.toString();

            return Double.valueOf(resStr);
        } catch (NumberFormatException exc) {
        }

        return 0;
    }

    public static String toWei(String number, int decimals) {
        if (number.isEmpty()) {
            return "0";
        }
        int dotPosition = number.indexOf(".");
        String multiplier = getDecimalsDepNumber(decimals);
        if (dotPosition != -1) {
            int zeroToRemove = number.length() - dotPosition - 1;
            multiplier = multiplier.substring(0, multiplier.length() - zeroToRemove);
            number = number.replace(".", "");
        }
        try {
            BigInteger bigNumber = new BigInteger(number, 10);
            BigInteger res = bigNumber.multiply(new BigInteger(multiplier));

            return res.toString();
        } catch (NumberFormatException ex) {
        }

        return "0";
    }

    public static double fromWei(String number, int decimals) {
        if (number == null || number.isEmpty()) {
            return 0;
        }
        BigInteger bigNumber = new BigInteger(number);
        BigInteger divider = new BigInteger(getDecimalsDepNumber(decimals));
        BigDecimal bDecimal = new BigDecimal(bigNumber);
        BigDecimal bDecimalRes = bDecimal.divide(new BigDecimal(divider), MathContext.DECIMAL32);
        String resStr = bDecimalRes.toPlainString();

        return Double.valueOf(resStr);
    }

    public static String toHexWei(String number, int decimals) {
        if (number.isEmpty()) {
            return "0x0";
        }
        int dotPosition = number.indexOf(".");
        String multiplier = getDecimalsDepNumber(decimals);
        if (dotPosition != -1) {
            int zeroToRemove = number.length() - dotPosition - 1;
            multiplier = multiplier.substring(0, multiplier.length() - zeroToRemove);
            number = number.replace(".", "");
        }
        BigInteger bigNumber = new BigInteger(number, 10);
        BigInteger res = bigNumber.multiply(new BigInteger(multiplier));

        return "0x" + res.toString(16);
    }

    public static String toHexWeiFromGWEI(String number) {
        try {
            if (number.isEmpty()) {
                return "0x0";
            }
            int dotPosition = number.indexOf(".");
            if (dotPosition != -1) {
                number = number.substring(0, dotPosition);
            }
            BigInteger bigNumber = new BigInteger(number, 10);
            return "0x" + bigNumber.toString(16);
        } catch (NumberFormatException exc) {
        }

        return "0x0";
    }

    public static String toWeiHex(String number) {
        if (number.isEmpty()) {
            return "0x0";
        }
        BigInteger bigNumber = new BigInteger(number, 10);

        return "0x" + bigNumber.toString(16);
    }

    public static String multiplyHexBN(String number1, String number2) {
        if (number1.startsWith("0x")) {
            number1 = number1.substring(2);
        }
        if (number2.startsWith("0x")) {
            number2 = number2.substring(2);
        }
        BigInteger bigNumber1 = new BigInteger(number1, 16);
        BigInteger bigNumber2 = new BigInteger(number2, 16);

        BigInteger res = bigNumber1.multiply(bigNumber2);

        return "0x" + res.toString(16);
    }

    public static byte[] hexStrToNumberArray(String value) {
        if (value.startsWith("0x")) {
            value = value.substring(2);
        }
        if (value.isEmpty()) {
            return new byte[0];
        }

        byte[] data = new byte[value.length() / 2];
        for (int n = 0; n < value.length(); n += 2) {
            data[n / 2] = (byte) Long.parseLong(value.substring(n, 2 + n), 16);
        }

        return data;
    }

    public static String numberArrayToHexStr(byte[] value) {
        if (value.length == 0) {
            return "";
        }
        String res = "0x";
        for (int n = 0; n < value.length; n++) {
            res += String.format("%02x", value[n]);
        }

        return res;
    }

    public static TxData getTxData(
            String nonce, String gasPrice, String gasLimit, String to, String value, byte[] data) {
        TxData res = new TxData();
        res.nonce = nonce;
        res.gasPrice = gasPrice;
        res.gasLimit = gasLimit;
        res.to = to;
        res.value = value;
        res.data = data;

        return res;
    }

    public static String stripAccountAddress(String address) {
        String newAddress = "";

        if (address.length() > 6) {
            newAddress = address.substring(0, 6) + "***" + address.substring(address.length() - 5);
        }

        return newAddress;
    }

    public static boolean isJSONValid(String text) {
        try {
            new JSONObject(text);
        } catch (JSONException ex) {
            try {
                new JSONArray(text);
            } catch (JSONException ex1) {
                return false;
            }
        }
        return true;
    }

    public static boolean isSwapLiquidityErrorReason(String error) {
        try {
            JSONObject mainObj = new JSONObject(error);
            JSONArray errorsArray = mainObj.getJSONArray("validationErrors");
            if (errorsArray == null) {
                return false;
            }
            for (int index = 0; index < errorsArray.length(); index++) {
                JSONObject errorObj = errorsArray.getJSONObject(index);
                if (errorObj == null) {
                    continue;
                }
                String reason = errorObj.getString("reason");
                if (reason.equals("INSUFFICIENT_ASSET_LIQUIDITY")) {
                    return true;
                }
            }
        } catch (JSONException ex) {
        }

        return false;
    }

    public static void setBitmapResource(ExecutorService executor, Handler handler, Context context,
            String iconPath, int iconId, ImageView iconImg, TextView textView) {
        if (iconPath == null) {
            if (iconImg != null) {
                iconImg.setImageResource(iconId);
            } else if (textView != null) {
                textView.setCompoundDrawablesRelativeWithIntrinsicBounds(
                        iconId, 0, R.drawable.ic_carat_down, 0);
            }

            return;
        }
        executor.execute(() -> {
            Uri logoFileUri = Uri.parse(iconPath);
            int resizeFactor = 110;
            if (textView != null) {
                resizeFactor = 70;
            }
            try (InputStream inputStream =
                            context.getContentResolver().openInputStream(logoFileUri)) {
                final Bitmap bitmap =
                        Utils.resizeBitmap(BitmapFactory.decodeStream(inputStream), resizeFactor);
                handler.post(() -> {
                    if (iconImg != null) {
                        iconImg.setImageBitmap(bitmap);
                    } else if (textView != null) {
                        textView.setCompoundDrawablesRelativeWithIntrinsicBounds(
                                new BitmapDrawable(context.getResources(), bitmap), null,
                                ApiCompatibilityUtils.getDrawable(
                                        context.getResources(), R.drawable.ic_carat_down),
                                null);
                    }
                });
            } catch (IOException exc) {
                org.chromium.base.Log.e("Utils", exc.getMessage());
            } catch (IllegalArgumentException exc) {
                org.chromium.base.Log.e("Utils", exc.getMessage());
            }
        });
    }

    public static void setBlockiesBitmapResource(ExecutorService executor, Handler handler,
            ImageView iconImg, String source, boolean makeLowerCase) {
        executor.execute(() -> {
            final Bitmap bitmap = Blockies.createIcon(source, makeLowerCase);
            handler.post(() -> {
                if (iconImg != null) {
                    iconImg.setImageBitmap(bitmap);
                }
            });
        });
    }

    public static Bitmap resizeBitmap(Bitmap source, int maxLength) {
        try {
            if (source.getHeight() >= source.getWidth()) {
                int targetHeight = maxLength;
                double aspectRatio = (double) source.getWidth() / (double) source.getHeight();
                int targetWidth = (int) (targetHeight * aspectRatio);

                Bitmap result = Bitmap.createScaledBitmap(source, targetWidth, targetHeight, false);
                return result;
            } else {
                int targetWidth = maxLength;
                double aspectRatio = ((double) source.getHeight()) / ((double) source.getWidth());
                int targetHeight = (int) (targetWidth * aspectRatio);

                Bitmap result = Bitmap.createScaledBitmap(source, targetWidth, targetHeight, false);
                return result;
            }
        } catch (Exception e) {
            return source;
        }
    }

    public static Double getOrDefault(
            HashMap<String, Double> map, String key, Double defaultValue) {
        // Can't use java.util.HashMap#getOrDefault with API level 21
        if (map.containsKey(key)) {
            return map.get(key);
        }
        return defaultValue;
    }

    public static <T> void removeIf(ArrayList<T> arrayList, Predicate<T> filter) {
        // Can't use java.util.ArrayList#removeIf with API level 21
        ArrayList<Integer> indexesToRemove = new ArrayList<Integer>();
        for (int i = 0; i < arrayList.size(); ++i) {
            if (filter.test(arrayList.get(i))) {
                indexesToRemove.add(i);
            }
        }
        if (indexesToRemove.isEmpty()) {
            return;
        }
        Collections.sort(indexesToRemove, Collections.reverseOrder());
        for (int i : indexesToRemove) {
            arrayList.remove(i);
        }
    }

    public static String getContractAddress(String chainId, String symbol, String contractAddress) {
        if (!chainId.equals(BraveWalletConstants.ROPSTEN_CHAIN_ID)) {
            return contractAddress;
        }
        if (symbol.equals("USDC")) {
            return "0x07865c6e87b9f70255377e024ace6630c1eaa37f";
        } else if (symbol.equals("DAI")) {
            return "0xad6d458402f60fd3bd25163575031acdce07538d";
        }

        return contractAddress;
    }

    public static int getTimeframeFromRadioButtonId(int radioButtonId) {
        if (radioButtonId == R.id.live_radiobutton) {
            return AssetPriceTimeframe.LIVE;
        } else if (radioButtonId == R.id.day_1_radiobutton) {
            return AssetPriceTimeframe.ONE_DAY;
        } else if (radioButtonId == R.id.week_1_radiobutton) {
            return AssetPriceTimeframe.ONE_WEEK;
        } else if (radioButtonId == R.id.month_1_radiobutton) {
            return AssetPriceTimeframe.ONE_MONTH;
        } else if (radioButtonId == R.id.month_3_radiobutton) {
            return AssetPriceTimeframe.THREE_MONTHS;
        } else if (radioButtonId == R.id.year_1_radiobutton) {
            return AssetPriceTimeframe.ONE_YEAR;
        } else {
            return AssetPriceTimeframe.ALL;
        }
    }

    public static String getTimeframeString(int assetPriceTimeframe) {
        Resources resources = ContextUtils.getApplicationContext().getResources();
        assert resources != null;

        switch (assetPriceTimeframe) {
            case AssetPriceTimeframe.LIVE:
                return resources.getString(R.string.trend_1h_text);
            case AssetPriceTimeframe.ONE_DAY:
                return resources.getString(R.string.trend_1d_text);
            case AssetPriceTimeframe.ONE_WEEK:
                return resources.getString(R.string.trend_1w_text);
            case AssetPriceTimeframe.ONE_MONTH:
                return resources.getString(R.string.trend_1m_text);
            case AssetPriceTimeframe.THREE_MONTHS:
                return resources.getString(R.string.trend_3m_text);
            case AssetPriceTimeframe.ONE_YEAR:
                return resources.getString(R.string.trend_1y_text);
            case AssetPriceTimeframe.ALL:
                return resources.getString(R.string.trend_all_text);
            default:
                assert false;
                return "N/A";
        }
    }

    public static ErcToken createEthereumErcToken() {
        ErcToken eth = new ErcToken();
        eth.name = "Ethereum";
        eth.symbol = "ETH";
        eth.contractAddress = "";
        eth.logo = "eth.png";
        eth.decimals = 18;
        return eth;
    }

    public static ErcToken[] fixupTokensRegistry(ErcToken[] tokens, String chainId) {
        for (ErcToken token : tokens) {
            token.contractAddress =
                    getContractAddress(chainId, token.symbol, token.contractAddress);
        }
        return tokens;
    }
}
