/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import static android.content.ClipDescription.MIMETYPE_TEXT_PLAIN;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.res.Resources;
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
import android.text.Html;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.ClickableSpan;
import android.text.style.URLSpan;
import android.util.Pair;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.annotation.StringRes;
import androidx.annotation.VisibleForTesting;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.hardware.fingerprint.FingerprintManagerCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletP3a;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.ProviderError;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.ChromeActivity;
import org.chromium.chrome.browser.app.domain.PortfolioModel;
import org.chromium.chrome.browser.crypto_wallet.activities.AssetDetailActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.NftDetailActivity;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.ApproveTxBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.listeners.OnWalletListItemClick;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.mojo.bindings.Callbacks;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.widget.Toast;
import org.chromium.url.GURL;

import java.io.InputStream;
import java.lang.Number;
import java.lang.NumberFormatException;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.math.MathContext;
import java.text.DateFormat;
import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.text.ParseException;
import java.text.ParsePosition;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ExecutorService;
import java.util.function.Predicate;

public class Utils {
    public static int ONBOARDING_FIRST_PAGE_ACTION = 1;
    public static int ONBOARDING_ACTION = 2;
    public static int UNLOCK_WALLET_ACTION = 3;
    public static int RESTORE_WALLET_ACTION = 4;

    public static int ACCOUNT_ITEM = 1;
    public static int ASSET_ITEM = 2;
    public static int TRANSACTION_ITEM = 3;

    public static final int ACCOUNT_REQUEST_CODE = 2;
    public static final int ETH_DEFAULT_DECIMALS = 18;
    public static final int SOL_DEFAULT_DECIMALS = 9;

    private static final String PREF_CRYPTO_ONBOARDING = "crypto_onboarding";
    public static final String DEX_AGGREGATOR_URL = "https://0x.org/";
    public static final String ENS_OFFCHAIN_LEARN_MORE_URL =
            "https://github.com/brave/brave-browser/wiki/ENS-offchain-lookup";
    public static final String BRAVE_SUPPORT_URL = "https://support.brave.com";
    public static final String ADDRESS = "address";
    public static final String NAME = "name";
    public static final String COIN_TYPE = "coinType";
    public static final String ISIMPORTED = "isImported";
    public static final String ISUPDATEACCOUNT = "isUpdateAccount";
    public static final String SWAP_EXCHANGE_PROXY = "0xdef1c0ded9bec7f1a1670819833240f027b25eff";
    public static final String ASSET_SYMBOL = "assetSymbol";
    public static final String ASSET_NAME = "assetName";
    public static final String ASSET_ID = "assetId";
    public static final String ASSET_CONTRACT_ADDRESS = "assetContractAddress";
    public static final String ASSET_LOGO = "assetLogo";
    public static final String ASSET_DECIMALS = "assetDecimals";
    public static final String CHAIN_ID = "chainId";
    public static final String IS_FROM_DAPPS = "isFromDapps";
    public static final String RESTART_WALLET_ACTIVITY = "restartWalletActivity";
    public static final String RESTART_WALLET_ACTIVITY_SETUP = "restartWalletActivitySetup";
    public static final String RESTART_WALLET_ACTIVITY_RESTORE = "restartWalletActivityRestore";
    public static final String ETHEREUM_CONTRACT_FOR_SWAP =
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    public static final BigInteger MAX_UINT256 =
            BigInteger.ONE.shiftLeft(256).subtract(BigInteger.ONE);

    // TODO(djandries): Add Filecoin when implemented
    public static int[] P3ACoinTypes = {CoinType.ETH, CoinType.SOL};

    private static final int CLEAR_CLIPBOARD_INTERVAL = 60000; // In milliseconds

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

    public static void saveTextToClipboard(
            Context context, String textToCopy, int textToShow, boolean scheduleClear) {
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
        clearClipboard(textToCopy, CLEAR_CLIPBOARD_INTERVAL);
    }

    public static String getTextFromClipboard(Context context) {
        ClipboardManager clipboard =
                (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clipData = clipboard.getPrimaryClip();
        String pasteData = "";
        if (clipData != null && clipData.getDescription().hasMimeType(MIMETYPE_TEXT_PLAIN)
                && clipData.getItemCount() > 0) {
            return clipData.getItemAt(0).getText().toString();
        }

        return "";
    }

    public static void clearClipboard(String textToCompare, int delay) {
        (new Timer()).schedule(new TimerTask() {
            @Override
            public void run() {
                String clipboardText = getTextFromClipboard(ContextUtils.getApplicationContext());
                if (textToCompare.equals(clipboardText)) {
                    saveTextToClipboard(ContextUtils.getApplicationContext(), "***", -1, false);
                }
            }
        }, delay);
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

    public static void hideKeyboard(Activity activity) {
        InputMethodManager imm =
                (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        View focusedView = activity.getCurrentFocus();
        if (focusedView != null) imm.hideSoftInputFromWindow(focusedView.getWindowToken(), 0);
    }

    public static void openBuySendSwapActivity(Activity activity,
            BuySendSwapActivity.ActivityType activityType, String swapFromAssetSymbol) {
        assert activity != null;
        Intent buySendSwapActivityIntent = new Intent(activity, BuySendSwapActivity.class);
        buySendSwapActivityIntent.putExtra("activityType", activityType.getValue());
        buySendSwapActivityIntent.putExtra("swapFromAssetSymbol", swapFromAssetSymbol);
        activity.startActivity(buySendSwapActivityIntent);
    }

    public static void openBuySendSwapActivity(
            Activity activity, BuySendSwapActivity.ActivityType activityType) {
        openBuySendSwapActivity(activity, activityType, null);
    }

    public static void openAssetDetailsActivity(
            Activity activity, String chainId, BlockchainToken asset) {
        assert activity != null;
        Intent assetDetailIntent = new Intent(activity, AssetDetailActivity.class);
        assetDetailIntent.putExtra(CHAIN_ID, chainId);
        assetDetailIntent.putExtra(ASSET_SYMBOL, asset.symbol);
        assetDetailIntent.putExtra(ASSET_NAME, asset.name);
        assetDetailIntent.putExtra(ASSET_ID, asset.tokenId);
        assetDetailIntent.putExtra(ASSET_LOGO, asset.logo);
        assetDetailIntent.putExtra(ASSET_CONTRACT_ADDRESS, asset.contractAddress);
        assetDetailIntent.putExtra(ASSET_DECIMALS, asset.decimals);
        assetDetailIntent.putExtra(COIN_TYPE, asset.coin);
        activity.startActivity(assetDetailIntent);
    }

    /**
     * Get a short name of a network
     * @param networkName of chain e.g. Ethereum Mainnet
     * @return short name of the network e.g. Ethereum
     */
    public static String getShortNameOfNetwork(String networkName) {
        if (!TextUtils.isEmpty(networkName)) {
            String firstWord = networkName.split(" ")[0];
            if (firstWord.length() > 18) {
                return firstWord.substring(0, 16) + "..";
            } else {
                return firstWord;
            }
        }
        return "";
    }

    public static void isCustomNetwork(JsonRpcService jsonRpcService, int coinType, String chainId,
            Callbacks.Callback1<Boolean> callback) {
        if (coinType != CoinType.ETH || jsonRpcService == null) {
            callback.call(false);
            return;
        }
        jsonRpcService.getCustomNetworks(coinType, chainIds -> {
            if (Arrays.asList(chainIds).contains(chainId))
                callback.call(true);
            else
                callback.call(false);
        });
    }

    public static boolean isDebuggable(Activity activity) {
        return 0 != (activity.getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE);
    }

    public static String[] makeNetworksAbbrevList(Activity activity, NetworkInfo[] allNetworks) {
        List<String> categories = new ArrayList<String>();

        for (NetworkInfo network : allNetworks) {
            // Disables localhost on Release builds
            if ((network.chainId.equals(BraveWalletConstants.LOCALHOST_CHAIN_ID)
                        && 0
                                != (activity.getApplicationInfo().flags
                                        & ApplicationInfo.FLAG_DEBUGGABLE))
                    || !network.chainId.equals(BraveWalletConstants.LOCALHOST_CHAIN_ID))
                categories.add(getNetworkShortText(network));
        }

        return categories.toArray(new String[0]);
    }

    public static NetworkInfo getNetworkInfoByChainId(String chainId, NetworkInfo[] allNetworks) {
        for (NetworkInfo network : allNetworks)
            if (network.chainId.equals(chainId)) return network;
        // Fall back to mainnet
        return allNetworks[0];
    }

    public static NetworkInfo[] getNetworkInfosByChainIds(
            String[] chainId, NetworkInfo[] allNetworks) {
        List<NetworkInfo> list = new ArrayList<NetworkInfo>();
        for (NetworkInfo network : allNetworks)
            if (Arrays.asList(chainId).contains(network.chainId)) list.add(network);

        return list.toArray(new NetworkInfo[0]);
    }

    public static NetworkInfo getNetworkInfoByName(String chainName, NetworkInfo[] allNetworks) {
        for (NetworkInfo network : allNetworks)
            if (network.chainName.equals(chainName)) return network;
        return allNetworks[0];
    }

    public static String getNetworkShortText(NetworkInfo network) {
        return getShortNameOfNetwork(network.chainName);
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

    // Supposedly toWei shall always end up with an integer
    private static BigInteger toWeiInternal(String number, int decimals) throws ParseException {
        NumberFormat nf = NumberFormat.getInstance(Locale.getDefault());
        ParsePosition parsePosition = new ParsePosition(0);
        BigDecimal parsed = null;
        if (nf instanceof DecimalFormat) {
            DecimalFormat df = (DecimalFormat) nf;
            df.setParseBigDecimal(true);
            parsed = (BigDecimal) df.parse(number, parsePosition);
        }

        if (parsed == null || parsePosition.getIndex() != number.length())
            throw new ParseException(
                    "Invalid input string to BigDecimal at ", parsePosition.getIndex());
        BigDecimal multiplier = BigDecimal.TEN.pow(decimals);
        return parsed.multiply(multiplier).toBigInteger();
    }

    public static String toWei(String number, int decimals, boolean calculateOtherAsset) {
        if (number.isEmpty() || calculateOtherAsset) {
            return "";
        }

        try {
            return toWeiInternal(number, decimals).toString();
        } catch (ParseException ex) {
            return "";
        }
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
            return "0x" + toWeiInternal(number, decimals).toString(16);
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

    public static String concatHexBN(String number1, String number2) {
        if (number1.startsWith("0x")) {
            number1 = number1.substring(2);
        }
        if (number2.startsWith("0x")) {
            number2 = number2.substring(2);
        }
        BigInteger bigNumber1 = new BigInteger(number1, 16);
        BigInteger bigNumber2 = new BigInteger(number2, 16);

        BigInteger res = bigNumber1.add(bigNumber2);

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

        if (hasPrefix)
            return prefix + value;
        else
            return value;
    }

    public static long toDecimalLamport(String amount, int decimals) {
        try {
            amount = removeHexPrefix(amount);
            BigDecimal value = new BigDecimal(amount);

            String resStr =
                    value.multiply(new BigDecimal(getDecimalsDepNumber(decimals))).toPlainString();
            int integerPlaces = resStr.indexOf('.');
            if (integerPlaces != -1 && (integerPlaces + 9) <= resStr.length()) {
                resStr = resStr.substring(0, integerPlaces + 9);
            }
            return (long) Double.parseDouble(resStr);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return 0;
    }

    public static String removeHexPrefix(String value) {
        if (value.startsWith("0x")) {
            return value.substring(2);
        }
        return value;
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
        res.signedTransaction = "";

        return res;
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

    public static void setBitmapResource(ExecutorService executor, Handler handler, Context context,
            String iconPath, int iconId, ImageView iconImg, TextView textView,
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
        executor.execute(() -> {
            Uri logoFileUri = Uri.parse(iconPath);
            int resizeFactorTemp = 110;
            if (textView != null) {
                resizeFactorTemp = 70;
            }
            final int resizeFactor = resizeFactorTemp;
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
                                drawCaratDown ? ApiCompatibilityUtils.getDrawable(
                                        context.getResources(), R.drawable.ic_carat_down)
                                              : null,
                                null);
                    }
                });
            } catch (Exception exc) {
                org.chromium.base.Log.e("Utils", exc.getMessage());
                if (textView != null) {
                    Drawable iconDrawable = AppCompatResources.getDrawable(context, iconId);
                    Bitmap bitmap = Bitmap.createBitmap(iconDrawable.getIntrinsicWidth(),
                            iconDrawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
                    Canvas canvas = new Canvas(bitmap);
                    iconDrawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
                    iconDrawable.draw(canvas);
                    Bitmap icon = Utils.resizeBitmap(bitmap, resizeFactor);
                    Drawable carat = ApiCompatibilityUtils.getDrawable(
                            context.getResources(), R.drawable.ic_carat_down);
                    handler.post(() -> {
                        textView.setCompoundDrawablesRelativeWithIntrinsicBounds(
                                new BitmapDrawable(context.getResources(), icon), null,
                                drawCaratDown ? carat : null, null);
                    });
                } else {
                    handler.post(() -> {
                        if (iconImg != null) {
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
        executor.execute(() -> {
            Bitmap bitmap1 = Blockies.createIcon(addresses[0], true);
            Bitmap bitmap2 = scaleDown(Blockies.createIcon(addresses[1], true), (float) 0.6);
            final Bitmap bitmap = overlayBitmapToCenter(bitmap1, bitmap2);
            handler.post(() -> {
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

    public static void setBlockiesBitmapCustomAsset(ExecutorService executor, Handler handler,
            ImageView iconImg, String source, String symbol, float scale, TextView textView,
            Context context, boolean drawCaratDown, float scaleDown) {
        executor.execute(() -> {
            final Bitmap bitmap =
                    drawTextToBitmap(scaleDown(Blockies.createIcon(source, true), scaleDown),
                            symbol.isEmpty() ? "" : symbol.substring(0, 1), scale, scaleDown);
            handler.post(() -> {
                if (iconImg != null) {
                    iconImg.setImageBitmap(bitmap);
                } else if (textView != null) {
                    textView.setCompoundDrawablesRelativeWithIntrinsicBounds(
                            new BitmapDrawable(context.getResources(), bitmap), null,
                            drawCaratDown ? ApiCompatibilityUtils.getDrawable(
                                    context.getResources(), R.drawable.ic_carat_down)
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
        int x = (bitmap.getWidth() - bounds.width()) / 2;
        int y = (bitmap.getHeight() + bounds.height()) / 2;
        canvas.drawText(text, x, y, paint);

        return bitmap;
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

    public static void setBlockiesBackground(ExecutorService executor, Handler handler, View view,
            String source, boolean makeLowerCase) {
        executor.execute(() -> {
            final Drawable background = Blockies.createBackground(source, makeLowerCase);
            handler.post(() -> {
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

    public static String getContractAddress(String chainId, String symbol, String contractAddress) {
        if (!chainId.equals(BraveWalletConstants.GOERLI_CHAIN_ID)) {
            return contractAddress;
        }
        if (symbol.equals("USDC")) {
            return "0x2f3a40a3db8a7e3d09b0adfefbce4f6f81927557";
        } else if (symbol.equals("DAI")) {
            return "0x73967c6a0904aa032c103b4104747e88c566b1a2";
        }

        return contractAddress;
    }

    public static String getGoerliContractAddress(String mainnetContractAddress) {
        String lowerCaseAddress = mainnetContractAddress.toLowerCase(Locale.getDefault());
        if (lowerCaseAddress.equals("0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48")) {
            return "0x2f3a40a3db8a7e3d09b0adfefbce4f6f81927557";
        } else if (lowerCaseAddress.equals("0x6b175474e89094c44da98b954eedeac495271d0f")) {
            return "0x73967c6a0904aa032c103b4104747e88c566b1a2";
        }

        return "";
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

    /*
     * Java port of the same function in components/brave_wallet_ui/options/asset-options.ts.
     */
    public static BlockchainToken makeNetworkAsset(NetworkInfo network) {
        String logo;

        // TODO: Add missing logos
        //             case "SOL":
        //                 logo = "sol.png";
        //                 break;
        //             case "FIL":
        //                 logo = "fil.png";
        //                 break;
        //             case network.chainId === BraveWallet.OPTIMISM_MAINNET_CHAIN_ID:
        //                 logo = "optimism.png";
        //                 break;
        //             case network.chainId === BraveWallet.AVALANCHE_MAINNET_CHAIN_ID:
        //                 logo = "avax.png";
        //                 break;
        //             case network.chainId === BraveWallet.FANTOM_MAINNET_CHAIN_ID:
        //                 logo = "fantom.png";
        //                 break;
        //             case network.chainId === BraveWallet.CELO_MAINNET_CHAIN_ID:
        //                 logo = "celo.png";
        //                 break;
        if (network.chainId.equals(BraveWalletConstants.MAINNET_CHAIN_ID)) {
            logo = "eth.png";
        } else if (network.chainId.equals(BraveWalletConstants.POLYGON_MAINNET_CHAIN_ID)) {
            logo = "matic.png";
        } else if (network.chainId.equals(
                           BraveWalletConstants.BINANCE_SMART_CHAIN_MAINNET_CHAIN_ID)) {
            logo = "bnb.png";
        } else if (network.chainId.equals(BraveWalletConstants.SOLANA_MAINNET)
                || network.chainId.equals(BraveWalletConstants.SOLANA_TESTNET)
                || network.chainId.equals(BraveWalletConstants.SOLANA_DEVNET)) {
            logo = "sol.png";
        } else {
            logo = "eth.png";
        }

        BlockchainToken asset = new BlockchainToken();
        asset.name = network.symbolName;
        asset.symbol = network.symbol;
        asset.contractAddress = "";
        asset.isErc20 = false;
        asset.isErc721 = false;
        asset.isNft = false;
        asset.logo = logo;
        asset.decimals = network.decimals;
        asset.visible = true;
        asset.chainId = network.chainId;
        asset.coin = network.coin;
        return asset;
    }

    // Replace USDC and DAI contract addresses for Goerli network
    public static BlockchainToken[] fixupTokensRegistry(BlockchainToken[] tokens, String chainId) {
        for (BlockchainToken token : tokens) {
            token.contractAddress =
                    getContractAddress(chainId, token.symbol, token.contractAddress);
        }
        return tokens;
    }

    public static AccountInfo findAccount(AccountInfo[] accounts, String address) {
        for (AccountInfo acc : accounts)
            if (acc.address.toLowerCase(Locale.getDefault())
                            .equals(address.toLowerCase(Locale.getDefault())))
                return acc;

        return null;
    }

    public static void openTransaction(TransactionInfo txInfo, JsonRpcService jsonRpcService,
            AppCompatActivity activity, String accountName, int coinType) {
        assert txInfo != null;
        if (txInfo.txStatus == TransactionStatus.UNAPPROVED) {
            if (activity instanceof ApprovedTxObserver) {
                showApproveDialog(txInfo, accountName, activity, ((ApprovedTxObserver) activity));
                return;
            }
            throw new RuntimeException("Activity must implement ApprovedTxObserver");
        } else {
            if (txInfo.txHash == null || txInfo.txHash.isEmpty()) {
                return;
            }
            openAddress("/tx/" + txInfo.txHash, jsonRpcService, activity, coinType);
        }
    }

    public static void openAddress(String toAppend, JsonRpcService jsonRpcService,
            AppCompatActivity activity, int coinType) {
        assert jsonRpcService != null;
        jsonRpcService.getNetwork(coinType, network -> {
            String blockExplorerUrl = Arrays.toString(network.blockExplorerUrls);
            if (blockExplorerUrl.length() > 2) {
                blockExplorerUrl = blockExplorerUrl.substring(1, blockExplorerUrl.length() - 1);
            }
            if (coinType == CoinType.ETH) {
                blockExplorerUrl += toAppend;
            } else if (coinType == CoinType.SOL) {
                int iPos = blockExplorerUrl.indexOf("?cluster=");
                if (iPos != -1) {
                    blockExplorerUrl = blockExplorerUrl.substring(0, iPos - 1) + toAppend
                            + blockExplorerUrl.substring(iPos);
                } else {
                    blockExplorerUrl += toAppend;
                }
            }
            TabUtils.openUrlInNewTab(false, blockExplorerUrl);
            TabUtils.bringChromeTabbedActivityToTheTop(activity);
        });
    }

    public static void openTransaction(TransactionInfo txInfo, JsonRpcService jsonRpcService,
            AppCompatActivity activity, AccountInfo[] accountInfos, int coinType) {
        assert txInfo != null;
        AccountInfo account = findAccount(accountInfos, txInfo.fromAddress);
        openTransaction(txInfo, jsonRpcService, activity,
                account != null ? account.name : stripAccountAddress(txInfo.fromAddress), coinType);
    }

    public static void setUpTransactionList(BraveWalletBaseActivity activity,
            AccountInfo[] accounts, WalletListItemModel walletListItemModel,
            HashMap<String, Double> assetPrices, BlockchainToken[] fullTokenList,
            HashMap<String, Double> nativeAssetsBalances,
            HashMap<String, HashMap<String, Double>> blockchainTokensBalances,
            RecyclerView rvTransactions, OnWalletListItemClick callback,
            WalletCoinAdapter walletTxCoinAdapter) {
        JsonRpcService jsonRpcService = activity.getJsonRpcService();
        TxService txService = activity.getTxService();
        assert jsonRpcService != null;
        assert txService != null;
        int coinType = CoinType.ETH;
        if (walletListItemModel.isAccount()) {
            AccountInfo account = findAccount(accounts, walletListItemModel.getSubTitle());
            coinType = account != null ? account.coin : CoinType.ETH;
        } else {
            coinType = walletListItemModel.getBlockchainToken().coin;
        }

        jsonRpcService.getNetwork(coinType, selectedNetwork -> {
            PendingTxHelper pendingTxHelper = new PendingTxHelper(txService, accounts, true);

            pendingTxHelper.fetchTransactions(() -> {
                HashMap<String, TransactionInfo[]> pendingTxInfos =
                        pendingTxHelper.getTransactions();
                pendingTxHelper.destroy();
                SolanaTransactionsGasHelper solanaTransactionsGasHelper =
                        new SolanaTransactionsGasHelper(
                                activity, getTransactionArray(pendingTxInfos));
                solanaTransactionsGasHelper.maybeGetSolanaGasEstimations(() -> {
                    workWithTransactions(activity, selectedNetwork, pendingTxInfos, accounts,
                            walletListItemModel, assetPrices, fullTokenList, nativeAssetsBalances,
                            blockchainTokensBalances, rvTransactions, callback, walletTxCoinAdapter,
                            solanaTransactionsGasHelper.getPerTxFee());
                });
            });
        });
    }

    private static TransactionInfo[] getTransactionArray(
            HashMap<String, TransactionInfo[]> txInfos) {
        TransactionInfo[] result = new TransactionInfo[0];
        for (String key : txInfos.keySet()) {
            TransactionInfo[] txs = txInfos.get(key);
            result = concatWithArrayCopy(result, txs);
        }

        return result;
    }

    private static <T> T[] concatWithArrayCopy(T[] array1, T[] array2) {
        T[] result = Arrays.copyOf(array1, array1.length + array2.length);
        System.arraycopy(array2, 0, result, array1.length, array2.length);

        return result;
    }

    private static void workWithTransactions(BraveWalletBaseActivity activity,
            NetworkInfo selectedNetwork, HashMap<String, TransactionInfo[]> pendingTxInfos,
            AccountInfo[] accounts, WalletListItemModel walletListItemModel,
            HashMap<String, Double> assetPrices, BlockchainToken[] fullTokenList,
            HashMap<String, Double> nativeAssetsBalances,
            HashMap<String, HashMap<String, Double>> blockchainTokensBalances,
            RecyclerView rvTransactions, OnWalletListItemClick callback,
            WalletCoinAdapter walletTxCoinAdapter, HashMap<String, Long> perTxSolanaFee) {
        walletTxCoinAdapter.setWalletCoinAdapterType(
                WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();

        BlockchainToken filterByToken =
                walletListItemModel.isAccount() ? null : walletListItemModel.getBlockchainToken();
        for (String accountName : pendingTxInfos.keySet()) {
            TransactionInfo[] txInfos = pendingTxInfos.get(accountName);
            for (TransactionInfo txInfo : txInfos) {
                long solanaEstimatedTxFee = 0;
                if (perTxSolanaFee.get(txInfo.id) != null) {
                    solanaEstimatedTxFee = perTxSolanaFee.get(txInfo.id);
                }
                ParsedTransaction parsedTx = ParsedTransaction.parseTransaction(txInfo,
                        selectedNetwork, accounts, assetPrices, solanaEstimatedTxFee, fullTokenList,
                        nativeAssetsBalances, blockchainTokensBalances);
                WalletListItemModel itemModel =
                        makeWalletItem((Context) activity, txInfo, selectedNetwork, parsedTx);
                // Filter by token. Account is already filtered in the accounts array.
                if (!walletListItemModel.isAccount()
                        && !walletListItemModel.getBlockchainToken().symbol.equals(
                                parsedTx.getSymbol()))
                    continue;
                walletListItemModelList.add(itemModel);
            }
        }
        walletTxCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletTxCoinAdapter.setOnWalletListItemClick(callback);
        walletTxCoinAdapter.setWalletListItemType(Utils.TRANSACTION_ITEM);
        rvTransactions.setAdapter(walletTxCoinAdapter);
        rvTransactions.setLayoutManager(new LinearLayoutManager((Context) activity));
    }

    private static WalletListItemModel makeWalletItem(Context context, TransactionInfo txInfo,
            NetworkInfo selectedNetwork, ParsedTransaction parsedTx) {
        Pair<String, String> itemTitles = parsedTx.makeTxListItemTitles(context);
        WalletListItemModel itemModel = new WalletListItemModel(
                R.drawable.ic_eth, itemTitles.first, itemTitles.second, "", null, null);
        updateWalletCoinTransactionStatus(itemModel, context, txInfo);

        itemModel.setChainSymbol(selectedNetwork.symbol);
        itemModel.setChainDecimals(selectedNetwork.decimals);
        itemModel.setTotalGas(parsedTx.getGasFee());
        itemModel.setTotalGasFiat(parsedTx.getGasFeeFiat());
        itemModel.setAddressesForBitmap(txInfo.fromAddress, parsedTx.getRecipient());
        itemModel.setTransactionInfo(txInfo);

        return itemModel;
    }

    public static void updateWalletCoinTransactionItem(
            WalletListItemModel item, TransactionInfo txInfo, Context context) {
        item.setTransactionInfo(txInfo);
        updateWalletCoinTransactionStatus(item, context, txInfo);
    }

    private static void showApproveDialog(TransactionInfo txInfo, String accountName,
            AppCompatActivity activity, ApprovedTxObserver approvedTxObserver) {
        ApproveTxBottomSheetDialogFragment approveTxBottomSheetDialogFragment =
                ApproveTxBottomSheetDialogFragment.newInstance(txInfo, accountName);
        approveTxBottomSheetDialogFragment.setApprovedTxObserver(approvedTxObserver);
        approveTxBottomSheetDialogFragment.show(activity.getSupportFragmentManager(),
                ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
    }

    private static void updateWalletCoinTransactionStatus(
            WalletListItemModel itemModel, Context context, TransactionInfo txInfo) {
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
            case TransactionStatus.ERROR:
            default:
                p.setColor(0xFFEE6374);
                txStatus = context.getResources().getString(R.string.wallet_tx_status_error);
        }
        itemModel.setTxStatus(txStatus);
        c.drawCircle(15, 15, 15, p);
        itemModel.setTxStatusBitmap(txStatusBitmap);
    }

    public static AlertDialog showPopUp(Context context, String title, String message,
            String positiveButtonTitle, int icon, DialogInterface.OnClickListener onClickListener) {
        assert null != context;
        MaterialAlertDialogBuilder builder =
                new MaterialAlertDialogBuilder(context, R.style.BraveWalletAlertDialogTheme)
                        .setTitle(title)
                        .setMessage(message)
                        .setIcon(icon);
        // positive button is only shown if the listener is not null
        if (null != onClickListener) {
            builder.setPositiveButton(positiveButtonTitle, onClickListener);
        }
        return builder.show();
    }

    public static SpannableString createSpannableString(
            String text, ClickableSpan clickListener, int startIndex, int endIndex, int flags) {
        assert null != text;
        SpannableString spannableString = new SpannableString(text);
        if (startIndex >= 0 && endIndex > startIndex && endIndex < text.length()) {
            spannableString.setSpan(clickListener, startIndex, endIndex, flags);
        }
        return spannableString;
    }

    /**
     * This method should be used to make substring of a string clickable
     * Example: This is <ph name="START">%1$s</ph>Clickable<ph name="END">%2$s</ph> text.
     *
     * @param context         The context
     * @param stringRes       The id of resource string
     * @param onClickListener The callback when clickable substring is clicked.
     */
    public static SpannableString createSpanForSurroundedPhrase(
            Context context, @StringRes int stringRes, View.OnClickListener onClickListener) {
        String htmlString =
                String.format(context.getResources().getString(stringRes), "<a href=\"\">", "</a>");
        SpannableString spannable = new SpannableString(AndroidUtils.formatHTML(htmlString));
        URLSpan[] spans = spannable.getSpans(0, spannable.length(), URLSpan.class);
        for (URLSpan urlSpan : spans) {
            NoUnderlineClickableSpan linkSpan = new NoUnderlineClickableSpan(
                    context, R.color.brave_link, (view) -> { onClickListener.onClick(view); });
            int spanStart = spannable.getSpanStart(urlSpan);
            int spanEnd = spannable.getSpanEnd(urlSpan);
            spannable.setSpan(linkSpan, spanStart, spanEnd, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            spannable.removeSpan(urlSpan);
        }
        return spannable;
    }

    public static SpannableString createSpannableString(
            String text, String spanText, ClickableSpan clickListener, int flags) {
        assert null != spanText;
        assert null != text;
        int startIndex = text.indexOf(spanText);
        int endIndex = startIndex + spanText.length();
        return createSpannableString(text, clickListener, startIndex, endIndex, flags);
    }

    public static void warnWhenError(
            String tag, String apiName, Integer error, String errorMessage) {
        if (error != ProviderError.SUCCESS) {
            Log.d(tag, apiName + ": " + error + " - " + errorMessage);
        }
    }

    public static Pair<Integer, String> getBuySendSwapContractAddress(BlockchainToken token) {
        int decimals = ETH_DEFAULT_DECIMALS;
        String address = ETHEREUM_CONTRACT_FOR_SWAP;
        if (token != null) {
            decimals = token.decimals;
            address = token.contractAddress;
            if (address.isEmpty()) address = ETHEREUM_CONTRACT_FOR_SWAP;
        }

        return new Pair<Integer, String>(decimals, address);
    }

    @RequiresApi(api = Build.VERSION_CODES.R)
    private static boolean canAuthenticate(BiometricManager biometricManager) {
        assert biometricManager != null;

        return biometricManager.canAuthenticate(BiometricManager.Authenticators.BIOMETRIC_WEAK)
                == BiometricManager.BIOMETRIC_SUCCESS;
    }

    @RequiresApi(api = Build.VERSION_CODES.P)
    public static boolean isBiometricAvailable(Context context) {
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
                return Utils.canAuthenticate(biometricManager);
            }

            return biometricManager.canAuthenticate() == BiometricManager.BIOMETRIC_SUCCESS;
        } else {
            // For API level < Q, we will use FingerprintManagerCompat to check enrolled
            // fingerprints. Note that for API level lower than 23, FingerprintManagerCompat behaves
            // like no fingerprint hardware and no enrolled fingerprints.
            FingerprintManagerCompat fingerprintManager = FingerprintManagerCompat.from(context);
            return fingerprintManager != null && fingerprintManager.isHardwareDetected()
                    && fingerprintManager.hasEnrolledFingerprints();
        }
    }

    public static String geteTLDHTMLFormatted(String etldPlusOne) {
        GURL url = getCurentTabUrl();

        return Utils.geteTLDHTMLFormatted(url, etldPlusOne);
    }

    public static Spanned geteTLD(String etldPlusOne) {
        GURL url = getCurentTabUrl();

        return Utils.geteTLD(url, etldPlusOne);
    }

    public static Spanned geteTLD(GURL url, String etldPlusOne) {
        String formattedeTLD = geteTLDHTMLFormatted(url, etldPlusOne);
        return AndroidUtils.formatHTML(formattedeTLD);
    }

    private static String geteTLDHTMLFormatted(GURL url, String etldPlusOne) {
        StringBuilder builder = new StringBuilder();
        builder.append(url.getScheme()).append("://").append(url.getHost());
        int index = builder.indexOf(etldPlusOne);
        if (index > 0 && index < builder.length()) {
            builder.insert(index, "<b>");
            builder.insert(builder.length(), "</b>");
        }
        return builder.toString();
    }

    public static GURL getCurentTabUrl() {
        ChromeTabbedActivity activity = BraveActivity.getChromeTabbedActivity();
        if (activity != null && activity.getActivityTab() != null) {
            return activity.getActivityTab().getUrl().getOrigin();
        }
        return GURL.emptyGURL();
    }

    @NonNull
    public static Profile getProfile(boolean isIncognito) {
        ChromeActivity chromeActivity = BraveActivity.getBraveActivity();
        if (chromeActivity == null) chromeActivity = BraveActivity.getChromeTabbedActivity();
        if (chromeActivity == null) return Profile.getLastUsedRegularProfile(); // Last resort

        return chromeActivity.getTabModelSelector().getModel(isIncognito).getProfile();
    }

    public static org.chromium.url.internal.mojom.Origin getCurrentMojomOrigin() {
        org.chromium.url.internal.mojom.Origin hostOrigin =
                new org.chromium.url.internal.mojom.Origin();
        ChromeTabbedActivity activity = BraveActivity.getChromeTabbedActivity();
        if (activity == null || activity.getActivityTab() == null) {
            return hostOrigin;
        }

        org.chromium.url.Origin urlOrigin =
                activity.getActivityTab().getWebContents().getMainFrame().getLastCommittedOrigin();
        if (urlOrigin == null) {
            return hostOrigin;
        }
        hostOrigin.scheme = urlOrigin.getScheme();
        hostOrigin.host = urlOrigin.getHost();
        hostOrigin.port = (short) urlOrigin.getPort();

        return hostOrigin;
    }

    public static WalletCoinAdapter setupVisibleAssetList(BlockchainToken[] userAssets,
            HashMap<String, Double> perTokenCryptoSum, HashMap<String, Double> perTokenFiatSum,
            String tokensPath) {
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();

        for (BlockchainToken userAsset : userAssets) {
            WalletListItemModel walletListItemModel = mapToWalletListItemModel(
                    perTokenCryptoSum, perTokenFiatSum, tokensPath, userAsset);
            walletListItemModelList.add(walletListItemModel);
        }

        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletCoinAdapter.setWalletListItemType(Utils.ASSET_ITEM);

        return walletCoinAdapter;
    }

    public static WalletCoinAdapter setupVisibleNftAssetList(
            List<PortfolioModel.NftDataModel> userAssets, HashMap<String, Double> perTokenCryptoSum,
            HashMap<String, Double> perTokenFiatSum, String tokensPath) {
        WalletCoinAdapter walletCoinAdapter =
                new WalletCoinAdapter(WalletCoinAdapter.AdapterType.VISIBLE_ASSETS_LIST);
        List<WalletListItemModel> walletListItemModelList = new ArrayList<>();

        for (PortfolioModel.NftDataModel userAsset : userAssets) {
            WalletListItemModel walletListItemModel = mapToWalletListItemModel(
                    perTokenCryptoSum, perTokenFiatSum, tokensPath, userAsset.token);
            walletListItemModel.setNftDataModel(userAsset);
            walletListItemModelList.add(walletListItemModel);
        }

        walletCoinAdapter.setWalletListItemModelList(walletListItemModelList);
        walletCoinAdapter.setWalletListItemType(Utils.ASSET_ITEM);

        return walletCoinAdapter;
    }

    @NonNull
    private static WalletListItemModel mapToWalletListItemModel(
            HashMap<String, Double> perTokenCryptoSum, HashMap<String, Double> perTokenFiatSum,
            String tokensPath, BlockchainToken userAsset) {
        String currentAssetKey = Utils.tokenToString(userAsset);
        Double fiatBalance = Utils.getOrDefault(perTokenFiatSum, currentAssetKey, 0.0d);
        String fiatBalanceString = String.format(Locale.getDefault(), "$%,.2f", fiatBalance);
        Double cryptoBalance = Utils.getOrDefault(perTokenCryptoSum, currentAssetKey, 0.0d);
        String cryptoBalanceString =
                String.format(Locale.getDefault(), "%.4f %s", cryptoBalance, userAsset.symbol);

        WalletListItemModel walletListItemModel =
                new WalletListItemModel(Utils.getCoinIcon(userAsset.coin), userAsset.name,
                        userAsset.symbol, userAsset.tokenId,
                        // Amount in USD
                        fiatBalanceString,
                        // Amount in current crypto currency/token
                        cryptoBalanceString);

        walletListItemModel.setIconPath("file://" + tokensPath + "/" + userAsset.logo);
        walletListItemModel.setBlockchainToken(userAsset);
        return walletListItemModel;
    }

    public static String formatErc721TokenTitle(String title, String id) {
        if (id.isEmpty() || id.equals("0")) {
            return title;
        }

        return title + " #" + id;
    }

    /**
     * Make a unique token title string in lower case.
     */
    public static String tokenToString(BlockchainToken token) {
        if (token == null) return "";

        final String symbolLowerCase = token.symbol.toLowerCase(Locale.getDefault());
        return token.isErc721 ? Utils.formatErc721TokenTitle(symbolLowerCase, token.tokenId)
                              : symbolLowerCase;
    }

    // Please only use this function when you need all the info (tokens, prices and balances) at the
    // same time!
    public static void getTxExtraInfo(BraveWalletBaseActivity activity, NetworkInfo[] allNetworks,
            NetworkInfo selectedNetwork, AccountInfo[] accountInfos,
            BlockchainToken[] filterByTokens, boolean userAssetsOnly,
            Callbacks.Callback4<HashMap<String, Double>, BlockchainToken[], HashMap<String, Double>,
                    HashMap<String, HashMap<String, Double>>> callback) {
        BraveWalletService braveWalletService = activity.getBraveWalletService();
        BlockchainRegistry blockchainRegistry = activity.getBlockchainRegistry();
        AssetRatioService assetRatioService = activity.getAssetRatioService();
        JsonRpcService jsonRpcService = activity.getJsonRpcService();
        assert braveWalletService != null && blockchainRegistry != null && assetRatioService != null
                && jsonRpcService != null;

        BraveWalletP3a braveWalletP3A = activity.getBraveWalletP3A();
        AsyncUtils.MultiResponseHandler multiResponse = new AsyncUtils.MultiResponseHandler(4);

        TokenUtils.getUserOrAllTokensFiltered(braveWalletService, blockchainRegistry,
                selectedNetwork, selectedNetwork.coin, TokenUtils.TokenType.ALL, userAssetsOnly,
                tokens -> {
                    final BlockchainToken[] fullTokenList = tokens;
                    if (filterByTokens != null) {
                        if (userAssetsOnly)
                            Log.w("Utils",
                                    "userAssetsOnly usually shouldn't be used with filterByTokens");
                        tokens = filterByTokens;
                    }

                    AsyncUtils.FetchPricesResponseContext fetchPricesContext =
                            new AsyncUtils.FetchPricesResponseContext(
                                    multiResponse.singleResponseComplete);
                    AssetsPricesHelper.fetchPrices(assetRatioService, tokens, fetchPricesContext);

                    AsyncUtils
                            .GetNativeAssetsBalancesResponseContext getNativeAssetsBalancesContext =
                            new AsyncUtils.GetNativeAssetsBalancesResponseContext(
                                    multiResponse.singleResponseComplete);
                    BalanceHelper.getNativeAssetsBalances(jsonRpcService, selectedNetwork,
                            accountInfos, getNativeAssetsBalancesContext);

                    AsyncUtils.GetBlockchainTokensBalancesResponseContext
                            getBlockchainTokensBalancesContext =
                            new AsyncUtils.GetBlockchainTokensBalancesResponseContext(
                                    multiResponse.singleResponseComplete);
                    BalanceHelper.getBlockchainTokensBalances(jsonRpcService, selectedNetwork,
                            accountInfos, tokens, getBlockchainTokensBalancesContext);

                    AsyncUtils.GetP3ABalancesContext getP3ABalancesContext =
                            new AsyncUtils.GetP3ABalancesContext(
                                    multiResponse.singleResponseComplete);
                    BalanceHelper.getP3ABalances(
                            activity, allNetworks, selectedNetwork, getP3ABalancesContext);

                    multiResponse.setWhenAllCompletedAction(() -> {
                        // P3A active accounts
                        HashMap<Integer, HashSet<String>> activeAddresses =
                                getP3ABalancesContext.activeAddresses;
                        BalanceHelper.updateActiveAddresses(
                                new AsyncUtils.GetNativeAssetsBalancesResponseContext[] {
                                        getNativeAssetsBalancesContext},
                                new AsyncUtils.GetBlockchainTokensBalancesResponseContext[] {
                                        getBlockchainTokensBalancesContext},
                                activeAddresses);
                        for (int coinType : P3ACoinTypes) {
                            braveWalletP3A.recordActiveWalletCount(
                                    activeAddresses.get(coinType).size(), coinType);
                        }

                        callback.call(fetchPricesContext.assetPrices, fullTokenList,
                                getNativeAssetsBalancesContext.nativeAssetsBalances,
                                getBlockchainTokensBalancesContext.blockchainTokensBalances);
                    });
                });
    }

    public static void getP3ANetworks(
            NetworkInfo[] allNetworks, Callbacks.Callback1<NetworkInfo[]> callback) {
        ArrayList<NetworkInfo> relevantNetworks = new ArrayList<NetworkInfo>();
        boolean countTestNetworks = CommandLine.getInstance().hasSwitch(
                BraveWalletConstants.P3A_COUNT_TEST_NETWORKS_SWITCH);
        for (NetworkInfo network : allNetworks) {
            // Exclude testnet chain data by default.
            // Testnet chain counting can be enabled via the
            // --p3a-count-wallet-test-networks switch
            if (countTestNetworks
                    || !WalletConstants.KNOWN_TEST_CHAIN_IDS.contains(network.chainId)) {
                relevantNetworks.add(network);
            }
        }

        callback.call(relevantNetworks.toArray(new NetworkInfo[0]));
    }

    public static boolean isNativeToken(NetworkInfo selectedNetwork, BlockchainToken token) {
        if (token.symbol.equals(selectedNetwork.symbol)) return true;
        return false;
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
                // TODO(sergz): Add FIL asset icon
                break;
            default:
                drawableId = R.drawable.ic_eth;
                break;
        }

        return drawableId;
    }

    public static String getKeyringForCoinType(int coinType) {
        String keyring = BraveWalletConstants.DEFAULT_KEYRING_ID;
        switch (coinType) {
            case CoinType.ETH:
                keyring = BraveWalletConstants.DEFAULT_KEYRING_ID;
                break;
            case CoinType.SOL:
                keyring = BraveWalletConstants.SOLANA_KEYRING_ID;
                break;
            case CoinType.FIL:
                keyring = BraveWalletConstants.FILECOIN_KEYRING_ID;
                break;
            default:
                keyring = BraveWalletConstants.DEFAULT_KEYRING_ID;
                break;
        }

        return keyring;
    }

    // TODO(sergz): Move getCoinIcon, getKeyringForCoinType, getBalanceForCoinType
    // to some kind of a separate Utils file that is related to diff networks only
    public static double getBalanceForCoinType(int coinType, int decimals, String balance) {
        double result = 0.0d;
        switch (coinType) {
            case CoinType.ETH:
                result = Utils.fromHexWei(balance, decimals);
                break;
            case CoinType.SOL:
                result = Utils.fromWei(balance, decimals);
                break;
            case CoinType.FIL:
                // TODO(sergz): Add FIL asset balance
                break;
            default:
                result = Utils.fromHexWei(balance, decimals);
                break;
        }

        return result;
    }

    public static boolean allowBuy(String chainId) {
        return !WalletConstants.KNOWN_TEST_CHAIN_IDS.contains(chainId)
                && WalletConstants.BUY_SUPPORTED_ONRAMP_NETWORKS.contains(chainId);
    }

    public static boolean allowSwap(String chainId) {
        return WalletConstants.SWAP_SUPPORTED_NETWORKS.contains(chainId);
    }

    public static double parseDouble(String s) throws ParseException {
        if (s.isEmpty()) return 0d;

        NumberFormat nf = NumberFormat.getNumberInstance(Locale.getDefault());
        ParsePosition parsePosition = new ParsePosition(0);
        Number num = nf.parse(s, parsePosition);

        if (parsePosition.getIndex() != s.length()) {
            throw new ParseException(
                    "Invalid input string to parseDouble at ", parsePosition.getIndex());
        }

        return num.doubleValue();
    }
}
