/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static android.content.ClipDescription.MIMETYPE_TEXT_PLAIN;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.hardware.biometrics.BiometricManager;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.URLSpan;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.annotation.VisibleForTesting;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.hardware.fingerprint.FingerprintManagerCompat;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.Callbacks;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave_wallet.mojom.AccountId;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.OriginInfo;
import org.chromium.brave_wallet.mojom.ProviderError;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.model.AccountSelectorItemModel;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.widget.Toast;

import java.io.InputStream;
import java.lang.ref.WeakReference;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.text.ParseException;
import java.text.ParsePosition;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.function.Predicate;
import java.util.stream.Collectors;

public class Utils {
    private static final String TAG = "Utils";

    public static final int ETH_DEFAULT_DECIMALS = 18;
    public static final int SOL_DEFAULT_DECIMALS = 9;

    private static final String PREF_CRYPTO_ONBOARDING = "crypto_onboarding";
    public static final String BRAVE_SUPPORT_URL = "https://support.brave.com";
    public static final String BRAVE_TERMS_OF_USE_URL = "https://brave.com/terms-of-use/";
    public static final String NAME = "name";
    public static final String COIN_TYPE = "coinType";
    public static final String SWAP_EXCHANGE_PROXY = "0xdef1c0ded9bec7f1a1670819833240f027b25eff";
    public static final String ASSET_SYMBOL = "assetSymbol";
    public static final String ASSET_NAME = "assetName";
    public static final String ASSET_ID = "assetId";
    public static final String CHAIN_ID = "chainId";

    public static final String ETHEREUM_CONTRACT_FOR_SWAP =
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    public static final BigInteger MAX_UINT256 =
            BigInteger.ONE.shiftLeft(256).subtract(BigInteger.ONE);

    public static int[] P3ACoinTypes = {CoinType.ETH, CoinType.SOL, CoinType.FIL};

    private static final int CLEAR_CLIPBOARD_INTERVAL = 60000; // In milliseconds

    @NonNull
    public static List<String> getRecoveryPhraseAsList(@NonNull final String recoveryPhrase) {
        final String[] recoveryPhraseArray = recoveryPhrase.split(" ");
        return new ArrayList<>(Arrays.asList(recoveryPhraseArray));
    }

    public static String getRecoveryPhraseFromList(List<String> recoveryPhrases) {
        String recoveryPhrasesText = "";
        for (String phrase : recoveryPhrases) {
            recoveryPhrasesText = recoveryPhrasesText.concat(phrase).concat(" ");
        }
        return recoveryPhrasesText.trim();
    }

    /**
     * Saves a given text to clipboard, shows a toast and clears it again after 60 seconds.
     *
     * @param context Context used to retrieve the clipboard service.
     * @param textToCopy Text that will be copied to clipboard.
     * @param textToShow String resource ID to display in the toast, or -1 to disable the toast.
     * @param scheduleClear {@code true} to clear the clipboard after {@link
     *     #CLEAR_CLIPBOARD_INTERVAL}.
     */
    public static void saveTextToClipboard(
            @NonNull final Context context,
            @NonNull final String textToCopy,
            @StringRes final int textToShow,
            final boolean scheduleClear) {
        ClipboardManager clipboard =
                (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clip = ClipData.newPlainText("", textToCopy);
        clipboard.setPrimaryClip(clip);
        if (textToShow != -1) {
            Toast.makeText(context, textToShow, Toast.LENGTH_SHORT).show();
        }
        if (!scheduleClear) {
            return;
        }

        PostTask.postDelayedTask(
                TaskTraits.UI_DEFAULT, () -> clearClipboard(textToCopy), CLEAR_CLIPBOARD_INTERVAL);
    }

    public static String getTextFromClipboard(Context context) {
        ClipboardManager clipboard =
                (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clipData = clipboard.getPrimaryClip();
        if (clipData != null
                && clipData.getDescription().hasMimeType(MIMETYPE_TEXT_PLAIN)
                && clipData.getItemCount() > 0) {
            return clipData.getItemAt(0).getText().toString();
        }

        return "";
    }

    /**
     * Clears the clipboard and replaces it with "***" if it matches a given text.
     *
     * @param textToCompare Text to compare that will trigger the clipboard clearing in case of a
     *     match.
     */
    public static void clearClipboard(@NonNull final String textToCompare) {
        String clipboardText = getTextFromClipboard(ContextUtils.getApplicationContext());
        if (textToCompare.equals(clipboardText)) {
            saveTextToClipboard(ContextUtils.getApplicationContext(), "***", -1, false);
        }
    }

    public static boolean shouldShowCryptoOnboarding() {
        SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();
        return mSharedPreferences.getBoolean(PREF_CRYPTO_ONBOARDING, true);
    }

    public static void setCryptoOnboarding(boolean enabled) {
        SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_CRYPTO_ONBOARDING, enabled);
        sharedPreferencesEditor.apply();
    }

    /**
     * Hides software keyboard.
     *
     * @param activity Activity used to retrieve input method service.
     */
    public static void hideKeyboard(@NonNull final Activity activity) {
        hideKeyboard(activity, null);
    }

    /**
     * Hides software keyboard targeting a specific window token, useful for those components
     * handling multiple views off screen (e.g. ViewPager2)
     *
     * @param activity Activity used to retrieve input method service.
     * @param windowToken Token of the window that is making the request.
     */
    public static void hideKeyboard(
            @NonNull final Activity activity, @Nullable final IBinder windowToken) {
        InputMethodManager imm =
                (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (windowToken != null) {
            imm.hideSoftInputFromWindow(windowToken, 0);
        } else {
            View focusedView = activity.getCurrentFocus();
            if (focusedView != null) {
                imm.hideSoftInputFromWindow(focusedView.getWindowToken(), 0);
            }
        }
    }

    /**
     * Get a short name of a network
     *
     * @param networkName of chain e.g. Ethereum Mainnet
     * @return short name of the network e.g. Ethereum
     */
    public static String getShortNameOfNetwork(String networkName) {
        if (!TextUtils.isEmpty(networkName)) {
            // If the length is small then show network name as it is.
            if (networkName.length() < 14) return networkName;
            String firstWord = networkName.split(" ")[0];
            if (firstWord.length() > 18) {
                return firstWord.substring(0, 16) + "..";
            } else {
                return firstWord;
            }
        }
        return "";
    }

    public static NetworkInfo getNetworkInfoByChainId(
            String chainId, List<NetworkInfo> allNetworks) {
        for (NetworkInfo network : allNetworks) {
            if (network.chainId.equals(chainId)) {
                return network;
            }
        }
        // Fall back to mainnet
        return allNetworks.get(0);
    }

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    public static String getDecimalsDepNumber(int decimals) {
        String strDecimals = "1";
        for (int i = 0; i < decimals; i++) {
            strDecimals += "0";
        }

        return strDecimals;
    }

    // Equivalent of Amount.divideByDecimals on desktop
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
        BigDecimal bDecimalRes = bDecimal.divide(new BigDecimal(divider));
        String resStr = bDecimalRes.toPlainString();
        int integerPlaces = resStr.indexOf('.');
        if (integerPlaces != -1 && (integerPlaces + 9) <= resStr.length()) {
            resStr = resStr.substring(0, integerPlaces + 9);
        }

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

    /**
     * Converts a given string to a big integer and multiplies the value of the object by ten raised
     * to the power of decimals.
     *
     * @param number Number to be multiplied, represented as a string.
     * @param decimals Number of decimals to multiply by.
     * @return The result of multiplying the number by ten raised to the power of decimals,
     *     expressed as a {@code BigInteger}.
     * @throws ParseException If the input string cannot be parsed as a {@code BigDecimal}.
     *     <p><b>Note:</b>: Supposedly, when converting to Wei the result shall always end up with
     *     an integer.
     */
    public static BigInteger multiplyByDecimals(@NonNull final String number, final int decimals)
            throws ParseException {
        NumberFormat nf = NumberFormat.getInstance(Locale.getDefault());
        ParsePosition parsePosition = new ParsePosition(0);
        BigDecimal parsed = null;
        if (nf instanceof DecimalFormat) {
            DecimalFormat df = (DecimalFormat) nf;
            df.setParseBigDecimal(true);
            parsed = (BigDecimal) df.parse(number, parsePosition);
        }

        if (parsed == null || parsePosition.getIndex() != number.length()) {
            throw new ParseException(
                    "Invalid input string to BigDecimal at ", parsePosition.getIndex());
        }
        BigDecimal multiplier = BigDecimal.TEN.pow(decimals);

        return parsed.multiply(multiplier).toBigInteger();
    }

    public static double fromWei(String number, int decimals) {
        if (number == null || number.isEmpty()) {
            return 0;
        }
        BigInteger bigNumber = new BigInteger(number);
        BigInteger divider = new BigInteger(getDecimalsDepNumber(decimals));
        BigDecimal bDecimal = new BigDecimal(bigNumber);
        BigDecimal bDecimalRes = bDecimal.divide(new BigDecimal(divider));
        String resStr = bDecimalRes.toPlainString();
        int integerPlaces = resStr.indexOf('.');
        if (integerPlaces != -1 && (integerPlaces + 9) <= resStr.length()) {
            resStr = resStr.substring(0, integerPlaces + 9);
        }

        return Double.valueOf(resStr);
    }

    public static String toHexWei(String number, int decimals) {
        if (number.isEmpty()) {
            return "0x0";
        }

        try {
            return "0x" + multiplyByDecimals(number, decimals).toString(16);
        } catch (ParseException ex) {
            return "0x0";
        }
    }

    public static String toHexGWeiFromGWEI(String number) {
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

    public static String toHex(String number) {
        if (number.isEmpty()) {
            return "0x0";
        }
        try {
            BigInteger bigNumber = new BigInteger(number, 10);
            return "0x" + bigNumber.toString(16);
        } catch (NumberFormatException e) {
            assert false;
        }
        return "0x0";
    }

    public static String hexToIntString(String number) {
        if (number.isEmpty()) {
            return "";
        }
        if (number.startsWith("0x")) {
            number = number.substring(2);
        }

        try {
            BigInteger bigNumber = new BigInteger(number, 16);
            return bigNumber.toString();
        } catch (NumberFormatException e) {
            assert false;
        }
        return "";
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
        if (value == null) {
            return new byte[0];
        }

        if (value.startsWith("0x")) {
            value = value.substring(2);
        }

        if (value.length() < 2 || (value.length() % 2 != 0)) {
            return new byte[0];
        }

        byte[] data = new byte[value.length() / 2];

        try {
            for (int n = 0; n < value.length(); n += 2) {
                data[n / 2] = (byte) Long.parseLong(value.substring(n, 2 + n), 16);
            }
        } catch (NumberFormatException ex) {
            return new byte[0];
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

    public static String maybeHexStrToUpperCase(String value) {
        if (value == null) {
            return "";
        }
        String prefix = "0x";
        boolean hasPrefix = false;
        if (value.startsWith(prefix)) {
            hasPrefix = true;
            value = value.substring(2);
        }

        value = value.toUpperCase(Locale.getDefault());

        if (hasPrefix) {
            return prefix + value;
        } else {
            return value;
        }
    }

    public static String stripAccountAddress(String address) {
        return address;
        // TODO(serg): Let's leave it for now as it could be we still
        // want to show a short address
        // String newAddress = "";

        // if (address.length() > 6) {
        //     newAddress = address.substring(0, 6) + "***" + address.substring(address.length() -
        //     5);
        // }

        // return newAddress;
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

    /**
     * Set the image either on {@code ImageView} or {@code TextView} based on given values. Try to
     * set @{code iconId} icon first otherwise create icon from token directory. Draw icon with
     * caret if @{code iconPath} is invalid.
     *
     * @param executor to run tasks in background
     * @param handler to run tasks on UI
     * @param context to access resources
     * @param iconPath to icon in token directory
     * @param iconId of a image in resources. Set icon if {@code iconPath} is null. Pass @{code
     *     Integer.MIN_VALUE} to avoid setting any default icon.
     * @param iconImg to set image if not null
     * @param textView to set image if not null
     * @param drawCaratDown true to draw a down caret before icon
     */
    public static void setBitmapResource(
            ExecutorService executor,
            Handler handler,
            Context context,
            String iconPath,
            int iconId,
            ImageView iconImg,
            TextView textView,
            boolean drawCaratDown) {
        if (iconPath == null) {
            if (iconImg != null) {
                iconImg.setImageResource(iconId);
            } else if (textView != null) {
                textView.setCompoundDrawablesRelativeWithIntrinsicBounds(
                        iconId, 0, R.drawable.ic_carat_down, 0);
            }

            return;
        }
        executor.execute(
                () -> {
                    Uri logoFileUri = Uri.parse(iconPath);
                    int resizeFactorTemp = 110;
                    if (textView != null) {
                        resizeFactorTemp = 70;
                    }
                    final int resizeFactor = resizeFactorTemp;
                    try (InputStream inputStream =
                            context.getContentResolver().openInputStream(logoFileUri)) {
                        final Bitmap bitmap =
                                Utils.resizeBitmap(
                                        BitmapFactory.decodeStream(inputStream), resizeFactor);
                        handler.post(
                                () -> {
                                    if (iconImg != null) {
                                        iconImg.setImageBitmap(bitmap);
                                    } else if (textView != null) {
                                        textView.setCompoundDrawablesRelativeWithIntrinsicBounds(
                                                new BitmapDrawable(context.getResources(), bitmap),
                                                null,
                                                drawCaratDown
                                                        ? ApiCompatibilityUtils.getDrawable(
                                                                context.getResources(),
                                                                R.drawable.ic_carat_down)
                                                        : null,
                                                null);
                                    }
                                });
                    } catch (Exception exc) {
                        org.chromium.base.Log.e("Utils", exc.getMessage());
                        if (textView != null) {
                            Drawable iconDrawable = AppCompatResources.getDrawable(context, iconId);
                            Bitmap bitmap =
                                    Bitmap.createBitmap(
                                            iconDrawable.getIntrinsicWidth(),
                                            iconDrawable.getIntrinsicHeight(),
                                            Bitmap.Config.ARGB_8888);
                            Canvas canvas = new Canvas(bitmap);
                            iconDrawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
                            iconDrawable.draw(canvas);
                            Bitmap icon = Utils.resizeBitmap(bitmap, resizeFactor);
                            Drawable carat =
                                    ApiCompatibilityUtils.getDrawable(
                                            context.getResources(), R.drawable.ic_carat_down);
                            handler.post(
                                    () -> {
                                        textView.setCompoundDrawablesRelativeWithIntrinsicBounds(
                                                new BitmapDrawable(context.getResources(), icon),
                                                null,
                                                drawCaratDown ? carat : null,
                                                null);
                                    });
                        } else {
                            handler.post(
                                    () -> {
                                        if (iconImg != null && iconId != Integer.MIN_VALUE) {
                                            iconImg.setImageResource(iconId);
                                        }
                                    });
                        }
                    }
                });
    }

    public static void overlayBitmaps(
            ExecutorService executor, Handler handler, String[] addresses, ImageView iconImg) {
        if (addresses == null || addresses.length != 2) {
            return;
        }
        executor.execute(
                () -> {
                    Bitmap bitmap1 = Blockies.createIcon(addresses[0], true, true);
                    Bitmap bitmap2 =
                            scaleDown(Blockies.createIcon(addresses[1], true, true), (float) 0.6);
                    final Bitmap bitmap = overlayBitmapToCenter(bitmap1, bitmap2);
                    handler.post(
                            () -> {
                                if (iconImg != null) {
                                    iconImg.setImageBitmap(bitmap);
                                }
                            });
                });
    }

    public static Bitmap scaleDown(Bitmap realImage, float ratio) {
        int width = Math.round((float) ratio * realImage.getWidth());
        int height = Math.round((float) ratio * realImage.getHeight());

        return Bitmap.createScaledBitmap(realImage, width, height, true);
    }

    public static Bitmap overlayBitmapToCenter(Bitmap bitmap1, Bitmap bitmap2) {
        int bitmap1Width = bitmap1.getWidth();
        int bitmap1Height = bitmap1.getHeight();

        float marginLeft = (float) (bitmap1Width * 0.6);
        float marginTop = (float) (bitmap1Height * 0.2);

        int newWidth = Math.round((float) (bitmap1Width * 1.4));
        Bitmap overlayBitmap = Bitmap.createBitmap(newWidth, bitmap1Height, bitmap1.getConfig());
        Canvas canvas = new Canvas(overlayBitmap);
        canvas.drawBitmap(bitmap1, new Matrix(), null);
        canvas.drawBitmap(bitmap2, marginLeft, marginTop, null);

        return overlayBitmap;
    }

    public static void setBlockiesBitmapCustomAsset(
            ExecutorService executor,
            Handler handler,
            ImageView iconImg,
            String source,
            String symbol,
            float scale,
            TextView textView,
            Context context,
            boolean drawCaratDown,
            float scaleDown,
            boolean circular) {
        executor.execute(
                () -> {
                    final Bitmap bitmap =
                            drawTextToBitmap(
                                    scaleDown(
                                            Blockies.createIcon(source, true, circular), scaleDown),
                                    symbol.isEmpty() ? "" : symbol.substring(0, 1),
                                    scale,
                                    scaleDown);
                    handler.post(
                            () -> {
                                if (iconImg != null) {
                                    iconImg.setImageBitmap(bitmap);
                                } else if (textView != null) {
                                    textView.setCompoundDrawablesRelativeWithIntrinsicBounds(
                                            new BitmapDrawable(context.getResources(), bitmap),
                                            null,
                                            drawCaratDown
                                                    ? ApiCompatibilityUtils.getDrawable(
                                                            context.getResources(),
                                                            R.drawable.ic_carat_down)
                                                    : null,
                                            null);
                                }
                            });
                });
    }

    public static Bitmap drawTextToBitmap(
            Bitmap bitmap, String text, float scale, float scaleDown) {
        Canvas canvas = new Canvas(bitmap);
        Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        paint.setColor(0xFF3D3D3D);
        paint.setTextSize((int) (24 * scale * scaleDown));

        Rect bounds = new Rect();
        paint.getTextBounds(text, 0, text.length(), bounds);
        float x = (bitmap.getWidth() - bounds.width()) / 2f;
        float y = (bitmap.getHeight() + bounds.height()) / 2f;
        canvas.drawText(text, x - bounds.left, y - bounds.bottom, paint);

        return bitmap;
    }

    public static void setTextGeneratedBlockies(
            ExecutorService executor,
            Handler handler,
            ImageView iconImg,
            String blockieSourceText,
            boolean makeLowerCase,
            boolean isCircular) {
        executor.execute(
                () -> {
                    final Bitmap bitmap =
                            Blockies.createIcon(blockieSourceText, makeLowerCase, isCircular);
                    handler.post(
                            () -> {
                                if (iconImg != null) {
                                    iconImg.setImageBitmap(bitmap);
                                }
                            });
                });
    }

    public static void setTextGeneratedBlockies(
            ExecutorService executor,
            Handler handler,
            ImageView iconImg,
            String blockieSourceText,
            boolean makeLowerCase) {
        setTextGeneratedBlockies(
                executor, handler, iconImg, blockieSourceText, makeLowerCase, true);
    }

    public static void setBlockiesBitmapResourceFromAccount(
            ExecutorService executor,
            Handler handler,
            ImageView iconImg,
            AccountInfo accountInfo,
            boolean makeLowerCase,
            boolean isCircular) {
        // TODO(apaymyshev): need to hash uniqueKey string for bitcoin accounts(same as for desktop)
        String source =
                accountInfo.address != null ? accountInfo.address : accountInfo.accountId.uniqueKey;
        setTextGeneratedBlockies(executor, handler, iconImg, source, makeLowerCase, isCircular);
    }

    public static void setBlockiesBitmapResourceFromAccount(
            ExecutorService executor,
            Handler handler,
            ImageView iconImg,
            AccountInfo accountInfo,
            boolean makeLowerCase) {
        // TODO(apaymyshev): need to hash uniqueKey string for bitcoin accounts(same as for desktop)
        setBlockiesBitmapResourceFromAccount(
                executor, handler, iconImg, accountInfo, makeLowerCase, true);
    }

    public static void setBlockiesBackground(
            ExecutorService executor,
            Handler handler,
            View view,
            String source,
            boolean makeLowerCase) {
        executor.execute(
                () -> {
                    final Drawable background = Blockies.createBackground(source, makeLowerCase);
                    handler.post(
                            () -> {
                                if (view != null) {
                                    view.setBackground(background);
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

    /*
     * java.util.HashMap#getOrDefault backport for API<24. Also checks null for the map.
     */
    public static <K, V> V getOrDefault(HashMap<K, V> map, K key, V defaultValue) {
        if (map == null) return defaultValue;
        if (map.containsKey(key)) {
            return map.get(key);
        }
        return defaultValue;
    }

    public static <T> void removeIf(List<T> list, Predicate<T> filter) {
        // Can't use java.util.ArrayList#removeIf with API level 21
        List<Integer> indexesToRemove = new ArrayList<Integer>();
        for (int i = 0; i < list.size(); ++i) {
            if (filter.test(list.get(i))) {
                indexesToRemove.add(i);
            }
        }
        if (indexesToRemove.isEmpty()) {
            return;
        }
        Collections.sort(indexesToRemove, Collections.reverseOrder());
        for (int i : indexesToRemove) {
            list.remove(i);
        }
    }

    /*
     * Java port of the same function in components/brave_wallet_ui/options/asset-options.ts.
     */
    public static BlockchainToken makeNetworkAsset(NetworkInfo network) {
        BlockchainToken asset = new BlockchainToken();
        asset.name = network.symbolName;
        asset.symbol = network.symbol;
        asset.contractAddress = "";
        asset.isErc20 = false;
        asset.isErc721 = false;
        asset.isNft = false;
        asset.logo = getNetworkIconName(network);
        asset.decimals = network.decimals;
        asset.visible = true;
        asset.chainId = network.chainId;
        asset.coin = network.coin;
        return asset;
    }

    @DrawableRes
    public static int getNetworkIconDrawable(
            @NonNull final String chainId, @CoinType.EnumType final int coin) {
        @DrawableRes int logo;
        switch (chainId) {
            case BraveWalletConstants.MAINNET_CHAIN_ID:
            case BraveWalletConstants.SEPOLIA_CHAIN_ID:
                logo = R.drawable.ic_eth_color;
                break;
            case BraveWalletConstants.POLYGON_MAINNET_CHAIN_ID:
                logo = R.drawable.ic_matic_color;
                break;
            case BraveWalletConstants.BNB_SMART_CHAIN_MAINNET_CHAIN_ID:
                logo = R.drawable.ic_bnbchain_color;
                break;
            case BraveWalletConstants.SOLANA_MAINNET:
            case BraveWalletConstants.SOLANA_TESTNET:
            case BraveWalletConstants.SOLANA_DEVNET:
                logo = R.drawable.ic_sol_color;
                break;
            case BraveWalletConstants.AURORA_MAINNET_CHAIN_ID:
                logo = R.drawable.ic_aurora_color;
                break;
            case BraveWalletConstants.ARBITRUM_MAINNET_CHAIN_ID:
                logo = R.drawable.ic_arb_color;
                break;
            case BraveWalletConstants.AVALANCHE_MAINNET_CHAIN_ID:
                logo = R.drawable.ic_avax_color;
                break;
            case BraveWalletConstants.CELO_MAINNET_CHAIN_ID:
                logo = R.drawable.ic_celo_color;
                break;
            case BraveWalletConstants.OPTIMISM_MAINNET_CHAIN_ID:
                logo = R.drawable.ic_op_color;
                break;
            case BraveWalletConstants.FANTOM_MAINNET_CHAIN_ID:
                logo = R.drawable.ic_ftm_color;
                break;
            case BraveWalletConstants.FILECOIN_MAINNET:
            case BraveWalletConstants.FILECOIN_TESTNET:
            case BraveWalletConstants.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID:
            case BraveWalletConstants.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID:
                logo = R.drawable.ic_filecoin_color;
                break;
            case BraveWalletConstants.BITCOIN_MAINNET:
            case BraveWalletConstants.BITCOIN_TESTNET:
                logo = R.drawable.ic_btc_color;
                break;
            case BraveWalletConstants.NEON_EVM_MAINNET_CHAIN_ID:
                logo = R.drawable.ic_neon_color;
                break;
            default:
                logo = -1;
        }
        // Local host chain is not unique per network
        if (logo == -1 && chainId.equals(BraveWalletConstants.LOCALHOST_CHAIN_ID)) {
            switch (coin) {
                case CoinType.SOL:
                    logo = R.drawable.ic_sol_color;
                    break;
                case CoinType.ETH:
                    logo = R.drawable.ic_eth_color;
                    break;
                case CoinType.FIL:
                    logo = R.drawable.ic_filecoin_color;
                    break;
                default: // Do nothing
            }
        }
        return logo;
    }

    @NonNull
    public static String getNetworkIconName(
            @NonNull final String chainId, @CoinType.EnumType final int coin) {
        String logo;
        switch (chainId) {
            case BraveWalletConstants.MAINNET_CHAIN_ID:
            case BraveWalletConstants.SEPOLIA_CHAIN_ID:
                logo = "eth.png";
                break;
            case BraveWalletConstants.POLYGON_MAINNET_CHAIN_ID:
                logo = "matic.png";
                break;
            case BraveWalletConstants.BNB_SMART_CHAIN_MAINNET_CHAIN_ID:
                logo = "bnb.png";
                break;
            case BraveWalletConstants.SOLANA_MAINNET:
            case BraveWalletConstants.SOLANA_TESTNET:
            case BraveWalletConstants.SOLANA_DEVNET:
                logo = "sol.png";
                break;
            case BraveWalletConstants.AURORA_MAINNET_CHAIN_ID:
                logo = "aurora.png";
                break;
            case BraveWalletConstants.ARBITRUM_MAINNET_CHAIN_ID:
                logo = "arb.png";
                break;
            case BraveWalletConstants.AVALANCHE_MAINNET_CHAIN_ID:
                logo = "avax.png";
                break;
            case BraveWalletConstants.CELO_MAINNET_CHAIN_ID:
                logo = "celo.png";
                break;
            case BraveWalletConstants.OPTIMISM_MAINNET_CHAIN_ID:
                logo = "op.png";
                break;
            case BraveWalletConstants.FANTOM_MAINNET_CHAIN_ID:
                logo = "ftm.png";
                break;
            case BraveWalletConstants.FILECOIN_MAINNET:
            case BraveWalletConstants.FILECOIN_TESTNET:
            case BraveWalletConstants.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID:
            case BraveWalletConstants.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID:
                logo = "fil.png";
                break;
            case BraveWalletConstants.BITCOIN_MAINNET:
            case BraveWalletConstants.BITCOIN_TESTNET:
                logo = "btc.png";
                break;
            case BraveWalletConstants.NEON_EVM_MAINNET_CHAIN_ID:
                logo = "neon.png";
                break;
            default:
                logo = "";
        }
        // Local host chain is not unique per network
        if (logo.isEmpty() && chainId.equals(BraveWalletConstants.LOCALHOST_CHAIN_ID)) {
            switch (coin) {
                case CoinType.SOL:
                    logo = "sol.png";
                    break;
                case CoinType.ETH:
                    logo = "eth.png";
                    break;
                case CoinType.FIL:
                    logo = "fil.png";
                    break;
                default: // Do nothing
            }
        }
        return logo;
    }

    @NonNull
    public static String getNetworkIconName(NetworkInfo network) {
        return Utils.getNetworkIconName(network.chainId, network.coin);
    }

    public static AccountInfo findAccountByAddress(AccountInfo[] accounts, String address) {
        for (AccountInfo acc : accounts) {
            if (acc.address
                    .toLowerCase(Locale.getDefault())
                    .equals(address.toLowerCase(Locale.getDefault()))) {
                return acc;
            }
        }

        return null;
    }

    public static AccountInfo findAccount(AccountInfo[] accounts, AccountId accountId) {
        for (AccountInfo acc : accounts) {
            if (WalletUtils.accountIdsEqual(acc.accountId, accountId)) {
                return acc;
            }
        }

        return null;
    }

    public static List<AccountInfo> filterAccountsByCoin(
            AccountInfo[] accounts, @CoinType.EnumType int coinType) {
        return Arrays.stream(accounts)
                .filter(account -> account.accountId.coin == coinType)
                .collect(Collectors.toList());
    }

    public static void openAddress(
            String toAppend, AppCompatActivity activity, int coinType, NetworkInfo networkInfo) {
        String blockExplorerUrl = Arrays.toString(networkInfo.blockExplorerUrls);
        if (blockExplorerUrl.length() > 2) {
            blockExplorerUrl = blockExplorerUrl.substring(1, blockExplorerUrl.length() - 1);
        }
        if (coinType == CoinType.ETH || coinType == CoinType.FIL) {
            blockExplorerUrl += toAppend;
        } else if (coinType == CoinType.SOL) {
            int iPos = blockExplorerUrl.indexOf("?cluster=");
            if (iPos != -1) {
                blockExplorerUrl =
                        blockExplorerUrl.substring(0, iPos - 1)
                                + toAppend
                                + blockExplorerUrl.substring(iPos);
            } else {
                blockExplorerUrl += toAppend;
            }
        }
        TabUtils.openUrlInCustomTab(activity, blockExplorerUrl);
    }

    public static void updateWalletCoinTransactionItem(
            AccountSelectorItemModel item, TransactionInfo txInfo, Context context) {
        item.setTransactionInfo(txInfo);
        updateWalletCoinTransactionStatus(item, context, txInfo);
    }

    public static void updateWalletCoinTransactionStatus(
            AccountSelectorItemModel itemModel, Context context, TransactionInfo txInfo) {
        String txStatus = context.getResources().getString(R.string.wallet_tx_status_unapproved);
        Bitmap txStatusBitmap = Bitmap.createBitmap(30, 30, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(txStatusBitmap);
        Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        switch (txInfo.txStatus) {
            case TransactionStatus.UNAPPROVED:
                p.setColor(0xFF5E6175);
                txStatus = context.getResources().getString(R.string.wallet_tx_status_unapproved);
                break;
            case TransactionStatus.APPROVED:
                p.setColor(0xFF2AC194);
                txStatus = context.getResources().getString(R.string.wallet_tx_status_approved);
                break;
            case TransactionStatus.REJECTED:
                p.setColor(0xFFEE6374);
                txStatus = context.getResources().getString(R.string.wallet_tx_status_rejected);
                break;
            case TransactionStatus.SUBMITTED:
                p.setColor(0xFFFFD43B);
                txStatus = context.getResources().getString(R.string.wallet_tx_status_submitted);
                break;
            case TransactionStatus.CONFIRMED:
                p.setColor(0xFF2AC194);
                txStatus = context.getResources().getString(R.string.wallet_tx_status_confirmed);
                break;
            case TransactionStatus.SIGNED:
                p.setColor(0xFFFFD43B);
                txStatus = context.getResources().getString(R.string.wallet_tx_status_signed);
                break;
            case TransactionStatus.ERROR:
            default:
                p.setColor(0xFFEE6374);
                txStatus = context.getResources().getString(R.string.wallet_tx_status_error);
        }
        itemModel.setTxStatus(txStatus);
        c.drawCircle(15, 15, 15, p);
        itemModel.setTxStatusBitmap(txStatusBitmap);
    }

    /**
     * This method should be used to make substring of a string clickable Example: This is <ph
     * name="START">%1$s</ph>Clickable<ph name="END">%2$s</ph> text.
     *
     * @param context The context
     * @param stringRes The id of resource string
     * @param onClickListener The callback when clickable substring is clicked.
     */
    @NonNull
    public static SpannableString createSpanForSurroundedPhrase(
            @NonNull Context context,
            @StringRes int stringRes,
            @NonNull View.OnClickListener onClickListener) {
        String htmlString =
                String.format(context.getResources().getString(stringRes), "<a href=\"\">", "</a>");
        SpannableString spannable = new SpannableString(AndroidUtils.formatHTML(htmlString));
        URLSpan[] spans = spannable.getSpans(0, spannable.length(), URLSpan.class);
        for (URLSpan urlSpan : spans) {
            NoUnderlineClickableSpan linkSpan =
                    new NoUnderlineClickableSpan(
                            context, R.color.brave_link, onClickListener::onClick);
            int spanStart = spannable.getSpanStart(urlSpan);
            int spanEnd = spannable.getSpanEnd(urlSpan);
            spannable.setSpan(linkSpan, spanStart, spanEnd, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            spannable.removeSpan(urlSpan);
        }
        return spannable;
    }

    public static void warnWhenError(
            String tag, String apiName, Integer error, String errorMessage) {
        if (error != ProviderError.SUCCESS) {
            Log.d(tag, apiName + ": " + error + " - " + errorMessage);
        }
    }

    @SuppressLint("MissingPermission")
    public static boolean isBiometricSupported(@Nullable Context context) {
        // Only Android versions 9 and above are supported.
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P || context == null) {
            return false;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            BiometricManager biometricManager = context.getSystemService(BiometricManager.class);
            if (biometricManager == null) {
                return false;
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                return biometricManager.canAuthenticate(
                                BiometricManager.Authenticators.BIOMETRIC_WEAK)
                        == BiometricManager.BIOMETRIC_SUCCESS;
            }

            //noinspection deprecation
            return biometricManager.canAuthenticate() == BiometricManager.BIOMETRIC_SUCCESS;
        } else {
            // For API level < Q, we will use FingerprintManagerCompat to check enrolled
            // fingerprints. Note that for API level lower than 23, FingerprintManagerCompat behaves
            // like no fingerprint hardware and no enrolled fingerprints.
            FingerprintManagerCompat fingerprintManager = FingerprintManagerCompat.from(context);
            return fingerprintManager.isHardwareDetected()
                    && fingerprintManager.hasEnrolledFingerprints();
        }
    }

    public static String geteTldHtmlString(OriginInfo originInfo) {
        StringBuilder builder = new StringBuilder();
        builder.append(originInfo.originSpec);
        int index = builder.lastIndexOf(originInfo.eTldPlusOne);
        if (index > 0 && index < builder.length()) {
            builder.insert(index, "<b>");
            builder.insert(builder.length(), "</b>");
        }
        return builder.toString();
    }

    public static Spanned geteTldSpanned(OriginInfo originInfo) {
        return AndroidUtils.formatHTML(geteTldHtmlString(originInfo));
    }

    @NonNull
    public static Profile getProfile(boolean isIncognito) {
        ChromeActivity chromeActivity = null;
        try {
            chromeActivity = BraveActivity.getBraveActivity();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "getProfile " + e);
        }
        if (chromeActivity == null) {
            chromeActivity = BraveActivity.getChromeTabbedActivity();
        }
        if (chromeActivity == null) {
            return ProfileManager.getLastUsedRegularProfile(); // Last resort
        }

        return chromeActivity.getTabModelSelector().getModel(isIncognito).getProfile();
    }

    public static String formatErc721TokenTitle(String title, String id) {
        if (id.isEmpty() || id.equals("0")) {
            return title;
        }

        return title + " #" + id;
    }

    /** Make a unique token title string in lower case. */
    public static String tokenToString(BlockchainToken token) {
        if (token == null) return "";

        final String symbolLowerCase = token.symbol.toLowerCase(Locale.ENGLISH);
        final String contractAddress = token.contractAddress.toLowerCase(Locale.ENGLISH);
        return JavaUtils.concatStrings(
                "#", symbolLowerCase, contractAddress, token.tokenId, token.chainId);
    }

    /**
     * Gets tokens, prices and balances, all at the same time for a given token type. See {@link
     * TokenUtils.TokenType}.
     *
     * @param activityRef Weak reference to Brave Wallet base activity.
     * @param tokenType Token type used for filtering (e.g. {@code TokenType.NON_NFTS}).
     * @param allNetworks List of all networks, used to log P3A records.
     * @param selectedNetwork Currently selected network.
     * @param accountInfos Array of account info.
     * @param filterByTokens Tokens used for fetching prices and balances. It may be {@code null}
     *     for fetching all tokens of a given token type. When {@code userAssetsOnly} is {@code
     *     true}, it should be set as {@code null}.
     * @param userAssetsOnly {@code true} for fetching only user assets. It should be used with
     *     {@code filterByTokens} set as {@code null}.
     * @param callback Callback containing four parameters: asset prices, token list, assets
     *     balances, blockchain token balances.
     *     <p><b>Note:</b>: Use this method wisely, and only if tokens, prices and balances are
     *     needed at the same time.
     */
    public static void getTxExtraInfo(
            WeakReference<BraveWalletBaseActivity> activityRef,
            TokenUtils.TokenType tokenType,
            List<NetworkInfo> allNetworks,
            NetworkInfo selectedNetwork,
            AccountInfo[] accountInfos,
            BlockchainToken[] filterByTokens,
            boolean userAssetsOnly,
            Callbacks.Callback4<
                            HashMap<String, Double>,
                            BlockchainToken[],
                            HashMap<String, Double>,
                            HashMap<String, HashMap<String, Double>>>
                    callback) {
        BraveWalletBaseActivity activity = activityRef.get();
        if (activity == null || activity.isFinishing()) {
            return;
        }
        BraveWalletService braveWalletService = activity.getBraveWalletService();
        BlockchainRegistry blockchainRegistry = activity.getBlockchainRegistry();
        AssetRatioService assetRatioService = activity.getAssetRatioService();
        JsonRpcService jsonRpcService = activity.getJsonRpcService();
        assert braveWalletService != null
                        && blockchainRegistry != null
                        && assetRatioService != null
                        && jsonRpcService != null
                : "Invalid service initialization";

        if (JavaUtils.anyNull(
                braveWalletService, blockchainRegistry, assetRatioService, jsonRpcService)) {
            return;
        }

        AsyncUtils.MultiResponseHandler multiResponse = new AsyncUtils.MultiResponseHandler(3);

        TokenUtils.getUserOrAllTokensFiltered(
                braveWalletService,
                blockchainRegistry,
                selectedNetwork,
                selectedNetwork.coin,
                tokenType,
                userAssetsOnly,
                tokens -> {
                    final BlockchainToken[] fullTokenList = tokens;
                    if (filterByTokens != null) {
                        if (userAssetsOnly) {
                            Log.w(
                                    "Utils",
                                    "userAssetsOnly usually shouldn't be used with filterByTokens");
                        }
                        tokens = filterByTokens;
                    }

                    AsyncUtils.FetchPricesResponseContext fetchPricesContext =
                            new AsyncUtils.FetchPricesResponseContext(
                                    multiResponse.singleResponseComplete);
                    AssetsPricesHelper.fetchPrices(assetRatioService, tokens, fetchPricesContext);

                    AsyncUtils.GetNativeAssetsBalancesResponseContext
                            getNativeAssetsBalancesContext =
                                    new AsyncUtils.GetNativeAssetsBalancesResponseContext(
                                            multiResponse.singleResponseComplete);
                    BalanceHelper.getNativeAssetsBalances(
                            jsonRpcService,
                            selectedNetwork,
                            accountInfos,
                            getNativeAssetsBalancesContext);

                    AsyncUtils.GetBlockchainTokensBalancesResponseContext
                            getBlockchainTokensBalancesContext =
                                    new AsyncUtils.GetBlockchainTokensBalancesResponseContext(
                                            multiResponse.singleResponseComplete);
                    BalanceHelper.getBlockchainTokensBalances(
                            jsonRpcService,
                            selectedNetwork,
                            accountInfos,
                            tokens,
                            getBlockchainTokensBalancesContext);

                    multiResponse.setWhenAllCompletedAction(
                            () -> {
                                callback.call(
                                        fetchPricesContext.assetPrices,
                                        fullTokenList,
                                        getNativeAssetsBalancesContext.nativeAssetsBalances,
                                        getBlockchainTokensBalancesContext
                                                .blockchainTokensBalances);
                                logP3ARecords(
                                        JavaUtils.asArray(getNativeAssetsBalancesContext),
                                        JavaUtils.asArray(getBlockchainTokensBalancesContext),
                                        activityRef,
                                        allNetworks,
                                        selectedNetwork);
                            });
                });
    }

    /**
     * Gets P3A networks (i.e. networks with chain Id contained in {@link
     * WalletConstants.KNOWN_TEST_CHAIN_IDS)} excluding testnet chains by default. Testnet chain
     * counting can be enabled using the switch `--p3a-count-wallet-test-networks`.
     *
     * @param allNetworks Given network list that will be filtered.
     * @param callback Callback containing a filtered list of P3A networks.
     */
    public static void getP3ANetworks(
            List<NetworkInfo> allNetworks, Callbacks.Callback1<List<NetworkInfo>> callback) {
        ArrayList<NetworkInfo> relevantNetworks = new ArrayList<NetworkInfo>();
        boolean countTestNetworks =
                CommandLine.getInstance()
                        .hasSwitch(BraveWalletConstants.P3A_COUNT_TEST_NETWORKS_SWITCH);
        for (NetworkInfo network : allNetworks) {
            // Exclude testnet chain data by default.
            // Testnet chain counting can be enabled via the
            // --p3a-count-wallet-test-networks switch
            if (countTestNetworks
                    || !WalletConstants.KNOWN_TEST_CHAIN_IDS.contains(network.chainId)) {
                relevantNetworks.add(network);
            }
        }

        callback.call(relevantNetworks);
    }

    public static boolean isNativeToken(NetworkInfo selectedNetwork, BlockchainToken token) {
        return token.symbol.equals(selectedNetwork.symbol);
    }

    public static int getCoinIcon(int coinType) {
        int drawableId = R.drawable.ic_eth;
        switch (coinType) {
            case CoinType.ETH:
                drawableId = R.drawable.ic_eth;
                break;
            case CoinType.SOL:
                drawableId = R.drawable.ic_sol_asset_icon;
                break;
            case CoinType.FIL:
                drawableId = R.drawable.ic_fil_asset_icon;
                break;
            default:
                drawableId = R.drawable.ic_eth;
                break;
        }

        return drawableId;
    }

    // TODO(sergz): Move getCoinIcon, getKeyringForEthOrSolOnly, getBalanceForCoinType
    // to some kind of a separate Utils file that is related to diff networks only
    public static double getBalanceForCoinType(int coinType, int decimals, String balance) {
        double result = 0.0d;
        switch (coinType) {
            case CoinType.ETH:
                result = Utils.fromHexWei(balance, decimals);
                break;
            case CoinType.SOL:
            case CoinType.FIL:
                result = Utils.fromWei(balance, decimals);
                break;
            default:
                result = Utils.fromHexWei(balance, decimals);
                break;
        }

        return result;
    }

    public static double parseDouble(String s) throws ParseException {
        if (s.isEmpty()) return 0d;

        NumberFormat nf = NumberFormat.getNumberInstance(Locale.getDefault());
        ParsePosition parsePosition = new ParsePosition(0);
        nf.setGroupingUsed(false);
        Number num = nf.parse(s, parsePosition);

        if (parsePosition.getIndex() != s.length()) {
            throw new ParseException(
                    "Invalid input string to parseDouble at ", parsePosition.getIndex());
        }

        return num.doubleValue();
    }

    private static void logP3ARecords(
            AsyncUtils.GetNativeAssetsBalancesResponseContext[] nativeAssetsBalancesResponses,
            AsyncUtils.GetBlockchainTokensBalancesResponseContext[]
                    blockchainTokensBalancesResponses,
            WeakReference<BraveWalletBaseActivity> activityRef,
            List<NetworkInfo> allNetworks,
            NetworkInfo selectedNetwork) {
        BraveWalletBaseActivity activity = activityRef.get();
        if (activity == null
                || activity.isFinishing()
                || JavaUtils.anyNull(activity.getBraveWalletP3A())) {
            return;
        }
        BraveWalletP3a braveWalletP3A = activity.getBraveWalletP3A();

        AsyncUtils.MultiResponseHandler multiResponse = new AsyncUtils.MultiResponseHandler(1);

        AsyncUtils.GetP3ABalancesContext getP3ABalancesContext =
                new AsyncUtils.GetP3ABalancesContext(multiResponse.singleResponseComplete);
        BalanceHelper.getP3ABalances(
                activityRef, allNetworks, selectedNetwork, getP3ABalancesContext);

        multiResponse.setWhenAllCompletedAction(
                () -> {
                    HashMap<Integer, HashSet<String>> activeAddresses =
                            getP3ABalancesContext.activeAddresses;
                    // P3A active accounts
                    BalanceHelper.updateActiveAddresses(
                            nativeAssetsBalancesResponses,
                            blockchainTokensBalancesResponses,
                            activeAddresses);
                    for (int coinType : P3ACoinTypes) {
                        braveWalletP3A.recordActiveWalletCount(
                                activeAddresses.get(coinType).size(), coinType);
                    }
                });
    }

    /**
     * Gets truncated address from a valid full contract address.
     *
     * @param address full contract address
     * @return truncated address
     */
    @NonNull
    public static String getTruncatedAddress(@NonNull final String address) {

        if (address.isEmpty()) {
            Log.w(TAG, "Empty contract address.");
            assert false;
            return "";
        }

        int prefixLength = address.startsWith("0x") ? 6 : 4;
        int lastFourCharactersIndex = address.length() - 4;

        if (lastFourCharactersIndex < 0 || prefixLength > address.length()) {
            Log.w(TAG, "Invalid contract address.");
            assert false;
            return "";
        }
        return (address.substring(0, prefixLength)
                + "***"
                + address.substring(address.length() - 4));
    }
}
