/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.chrome.browser.crypto_wallet.util.Utils.warnWhenError;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextWatcher;
import android.text.method.LinkMovementMethod;
import android.util.DisplayMetrics;
import android.util.Pair;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.RelativeLayout;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.vision.MultiProcessor;
import com.google.android.gms.vision.barcode.Barcode;
import com.google.android.gms.vision.barcode.BarcodeDetector;

import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainRegistry;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.BraveWalletService;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.GasEstimation1559;
import org.chromium.brave_wallet.mojom.JsonRpcService;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.KeyringService;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.ProviderError;
import org.chromium.brave_wallet.mojom.SwapParams;
import org.chromium.brave_wallet.mojom.SwapResponse;
import org.chromium.brave_wallet.mojom.SwapService;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.brave_wallet.mojom.TxDataUnion;
import org.chromium.brave_wallet.mojom.TxService;
import org.chromium.brave_wallet.mojom.TxServiceObserver;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.JsonRpcServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.KeyringServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.SwapServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.TxServiceFactory;
import org.chromium.chrome.browser.crypto_wallet.adapters.AccountSpinnerAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSpinnerAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.ApproveTxBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.EditVisibleAssetsBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.observers.KeyringServiceObserver;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.Validations;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.qrreader.BarcodeTracker;
import org.chromium.chrome.browser.qrreader.BarcodeTrackerFactory;
import org.chromium.chrome.browser.qrreader.CameraSource;
import org.chromium.chrome.browser.qrreader.CameraSourcePreview;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.text.NoUnderlineClickableSpan;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BuySendSwapActivity extends BraveWalletBaseActivity
        implements AdapterView.OnItemSelectedListener, BarcodeTracker.BarcodeGraphicTrackerCallback,
                   ApprovedTxObserver {
    private final static String TAG = "BuySendSwapActivity";
    private static final int RC_HANDLE_CAMERA_PERM = 113;
    // Intent request code to handle updating play services if needed.
    private static final int RC_HANDLE_GMS = 9001;

    private CameraSource mCameraSource;
    private CameraSourcePreview mCameraSourcePreview;
    private boolean mInitialLayoutInflationComplete;

    private int radioSlippageToleranceCheckedId;
    private TextView mMarketLimitPriceText;

    public enum ActivityType {
        BUY(0),
        SEND(1),
        SWAP(2);

        private int value;
        private static Map map = new HashMap<>();

        private ActivityType(int value) {
            this.value = value;
        }

        static {
            for (ActivityType activityType : ActivityType.values()) {
                map.put(activityType.value, activityType);
            }
        }

        public static ActivityType valueOf(int activityType) {
            return (ActivityType) map.get(activityType);
        }

        public int getValue() {
            return value;
        }
    }

    public String mActivateAllowanceTxId;

    private ActivityType mActivityType;
    private AccountSpinnerAdapter mCustomAccountAdapter;
    private double mConvertedFromBalance;
    private double mConvertedToBalance;
    private BlockchainToken mCurrentBlockchainToken;
    private BlockchainToken mCurrentSwapToBlockchainToken;
    private SwapService mSwapService;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private String mCurrentChainId;
    private String mAllowanceTarget;
    private Spinner mAccountSpinner;
    private Button mBtnBuySendSwap;
    private TextView mSlippageToleranceText;
    private EditText mFromValueText;
    private EditText mToValueText;
    private EditText mSendToAddrText;
    private TextView mMarketPriceValueText;
    private TextView mFromBalanceText;
    private TextView mToBalanceText;
    private TextView mFromAssetText;
    private TextView mToAssetText;
    private TextView mSendToValidation;
    private TextView mFromSendValueValidation;

    @Override
    public void onDestroy() {
        if (mCameraSourcePreview != null) {
            mCameraSourcePreview.release();
        }
        mSwapService.close();
        super.onDestroy();
    }

    @Override
    @SuppressLint("SetTextI18n")
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_buy_send_swap);

        mAccountSpinner = findViewById(R.id.accounts_spinner);
        mActivateAllowanceTxId = "";
        Intent intent = getIntent();
        mActivityType = ActivityType.valueOf(
                intent.getIntExtra("activityType", ActivityType.BUY.getValue()));

        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mFromValueText = findViewById(R.id.from_value_text);
        mFromValueText.setText("");
        mFromValueText.setHint("0");

        mFromBalanceText = findViewById(R.id.from_balance_text);
        mToBalanceText = findViewById(R.id.to_balance_text);
        mFromAssetText = findViewById(R.id.from_asset_text);
        mToAssetText = findViewById(R.id.to_asset_text);

        mMarketLimitPriceText = findViewById(R.id.market_limit_price_text);
        mMarketPriceValueText = findViewById(R.id.market_price_value_text);

        mSlippageToleranceText = findViewById(R.id.slippage_tolerance_dropdown);

        mBtnBuySendSwap = findViewById(R.id.btn_buy_send_swap);
        mToValueText = findViewById(R.id.to_value_text);

        mSendToValidation = findViewById(R.id.to_send_error_text);
        mFromSendValueValidation = findViewById(R.id.from_send_value_error_text);

        onInitialLayoutInflationComplete();
        mInitialLayoutInflationComplete = true;

        adjustControls();
    }

    private class BuySendSwapUiInfo {
        protected boolean shouldShowBuyControls;
        protected String titleText;
        protected String secondText;
        protected String buttonText;
        protected String linkUrl;
    }

    private BuySendSwapUiInfo getPerNetworkUiInfo(String chainId) {
        BuySendSwapUiInfo buySendSwapUiInfo = new BuySendSwapUiInfo();

        if (chainId.equals(BraveWalletConstants.MAINNET_CHAIN_ID)) {
            buySendSwapUiInfo.shouldShowBuyControls = true;
            buySendSwapUiInfo.buttonText = getString(R.string.wallet_buy_mainnet_button_text);
            return buySendSwapUiInfo;
        }

        buySendSwapUiInfo.shouldShowBuyControls = false;
        buySendSwapUiInfo.titleText = getString(R.string.wallet_test_faucet_title);
        buySendSwapUiInfo.buttonText = getString(R.string.wallet_test_faucet_button_text);

        buySendSwapUiInfo.secondText = getString(R.string.wallet_test_faucet_second_text);
        buySendSwapUiInfo.secondText = String.format(
                buySendSwapUiInfo.secondText, Utils.getNetworkShortText(this, chainId));

        buySendSwapUiInfo.linkUrl = Utils.getBuyUrlForTestChain(chainId);

        return buySendSwapUiInfo;
    }

    private void adjustTestFaucetControls(BuySendSwapUiInfo buySendSwapUiInfo) {
        View testFaucetsBlock = findViewById(R.id.test_faucets_block);
        assert testFaucetsBlock != null;
        View paymentParamsBlock = findViewById(R.id.payment_params_block);
        assert paymentParamsBlock != null;

        if (buySendSwapUiInfo.shouldShowBuyControls) {
            paymentParamsBlock.setVisibility(View.VISIBLE);
            testFaucetsBlock.setVisibility(View.GONE);
        } else {
            paymentParamsBlock.setVisibility(View.GONE);
            testFaucetsBlock.setVisibility(View.VISIBLE);
            ((TextView) findViewById(R.id.test_faucet_tittle)).setText(buySendSwapUiInfo.titleText);
            ((TextView) findViewById(R.id.test_faucet_message))
                    .setText(buySendSwapUiInfo.secondText);
        }
        mBtnBuySendSwap.setText(buySendSwapUiInfo.buttonText);
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        if (parent.getId() == R.id.network_spinner) {
            String item = parent.getItemAtPosition(position).toString();

            if (mJsonRpcService != null) {
                mJsonRpcService.getAllNetworks(CoinType.ETH, chains -> {
                    NetworkInfo[] customNetworks = Utils.getCustomNetworks(chains);
                    final String chainId = Utils.getNetworkConst(this, item, customNetworks);

                    if (mActivityType == ActivityType.BUY) {
                        adjustTestFaucetControls(getPerNetworkUiInfo(chainId));
                    }

                    // Shall be fine regardless of activity type
                    mFromValueText.setText("");
                    mFromValueText.setHint("0");
                    resetSwapFromToAssets();

                    mJsonRpcService.setNetwork(chainId, CoinType.ETH, (success) -> {
                        if (!success) {
                            Log.e(TAG, "Could not set network");
                        }
                        mCurrentChainId = chainId;
                    });
                    updateBalanceMaybeSwap(getCurrentSelectedAccountAddr());
                });
            }
        } else if (parent.getId() == R.id.accounts_spinner) {
            updateBalanceMaybeSwap(mCustomAccountAdapter.getTitleAtPosition(position));
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> arg0) {}

    private void getSendSwapQuota(boolean calculatePerSellAsset, boolean sendTx) {
        String from =
                mCustomAccountAdapter.getTitleAtPosition(mAccountSpinner.getSelectedItemPosition());
        Pair<Integer, String> toInfo =
                Utils.getBuySendSwapContractAddress(mCurrentSwapToBlockchainToken);
        Pair<Integer, String> fromInfo =
                Utils.getBuySendSwapContractAddress(mCurrentBlockchainToken);
        String percent = mSlippageToleranceText.getText().toString().replace("%", "");
        SwapParams swapParams =
                getSwapParams(from, fromInfo, toInfo, percent, calculatePerSellAsset);
        if (swapParams == null) return;

        assert mSwapService != null;
        if (!sendTx) {
            mSwapService.getPriceQuote(swapParams, (success, response, error_response) -> {
                workWithSwapQuota(
                        success, response, error_response, calculatePerSellAsset, sendTx, from);
            });
        } else {
            mSwapService.getTransactionPayload(swapParams, (success, response, error_response) -> {
                workWithSwapQuota(
                        success, response, error_response, calculatePerSellAsset, sendTx, from);
            });
        }
    }

    private SwapParams getSwapParams(String from, Pair<Integer, String> fromInfo,
            Pair<Integer, String> toInfo, String percent, boolean calculatePerSellAsset) {
        String value = mFromValueText.getText().toString();
        String valueTo = mToValueText.getText().toString();

        SwapParams swapParams = new SwapParams();
        swapParams.takerAddress = from;
        swapParams.sellAmount = Utils.toWei(value, fromInfo.first, !calculatePerSellAsset);
        swapParams.buyAmount = Utils.toWei(valueTo, toInfo.first, calculatePerSellAsset);
        if (swapParams.sellAmount.isEmpty() && swapParams.buyAmount.isEmpty()) {
            mBtnBuySendSwap.setEnabled(false);
            mBtnBuySendSwap.setText(getString(R.string.swap));

            return null;
        }
        swapParams.buyToken = toInfo.second;
        swapParams.sellToken = fromInfo.second;
        try {
            swapParams.slippagePercentage = Double.parseDouble(percent) / 100;
        } catch (NumberFormatException ex) {
        }
        swapParams.gasPrice = "";

        return swapParams;
    }

    private void workWithSwapQuota(boolean success, SwapResponse response, String errorResponse,
            boolean calculatePerSellAsset, boolean sendTx, String from) {
        if (!success) {
            response = new SwapResponse();
            response.sellAmount = "0";
            response.buyAmount = "0";
            response.price = "0";
            if (errorResponse != null) {
                if (sendTx) {
                    Log.e(TAG, "Swap error: " + errorResponse);
                }
            }
            mBtnBuySendSwap.setEnabled(true);
        } else {
            if (sendTx) {
                TxData data = Utils.getTxData("", Utils.toWeiHex(response.gasPrice),
                        Utils.toWeiHex(response.estimatedGas), response.to,
                        Utils.toWeiHex(response.value), Utils.hexStrToNumberArray(response.data));
                sendSwapTransaction(data, from);
            }
        }
        updateSwapControls(response, calculatePerSellAsset, errorResponse);
    }

    private void sendSwapTransaction(TxData data, String from) {
        assert mEthTxManagerProxy != null;
        mEthTxManagerProxy.getGasEstimation1559(estimation -> {
            String maxPriorityFeePerGas = "";
            String maxFeePerGas = "";
            if (estimation.fastMaxPriorityFeePerGas.equals(estimation.avgMaxPriorityFeePerGas)) {
                // Bump fast priority fee and max fee by 1 GWei if same as average fees.
                maxPriorityFeePerGas = Utils.concatHexBN(
                        estimation.fastMaxPriorityFeePerGas, Utils.toHexWei("1", 9));
                maxFeePerGas =
                        Utils.concatHexBN(estimation.fastMaxFeePerGas, Utils.toHexWei("1", 9));
            } else {
                // Always suggest fast gas fees as default
                maxPriorityFeePerGas = estimation.fastMaxPriorityFeePerGas;
                maxFeePerGas = estimation.fastMaxFeePerGas;
            }
            data.gasPrice = "";
            sendTransaction(data, from, maxPriorityFeePerGas, maxFeePerGas);
        });
    }

    private void updateSwapControls(
            SwapResponse response, boolean calculatePerSellAsset, String errorResponse) {
        int decimals = 18;
        if (!calculatePerSellAsset) {
            if (mCurrentBlockchainToken != null) {
                decimals = mCurrentBlockchainToken.decimals;
            }
            mFromValueText.setText(String.format(
                    Locale.getDefault(), "%.4f", Utils.fromWei(response.sellAmount, decimals)));
        } else {
            if (mCurrentSwapToBlockchainToken != null) {
                decimals = mCurrentSwapToBlockchainToken.decimals;
            }
            mToValueText.setText(String.format(
                    Locale.getDefault(), "%.4f", Utils.fromWei(response.buyAmount, decimals)));
        }
        if (calculatePerSellAsset) {
            mMarketPriceValueText.setText(response.price);
        } else {
            try {
                double price = Double.parseDouble(response.price);
                if (price != 0) {
                    price = 1 / price;
                }
                mMarketPriceValueText.setText(String.format(Locale.getDefault(), "%.18f", price));
            } catch (NumberFormatException | NullPointerException ex) {
            }
        }
        String symbol = "ETH";
        if (mCurrentBlockchainToken != null) {
            symbol = mCurrentBlockchainToken.symbol;
        }
        mMarketLimitPriceText.setText(String.format(getString(R.string.market_price_in), symbol));
        checkBalanceShowError(response, errorResponse);
    }

    private void resetSwapFromToAssets() {
        if (mActivityType == ActivityType.BUY) return;
        if (mBlockchainRegistry != null && mCustomAccountAdapter != null
                && mInitialLayoutInflationComplete) {
            final BlockchainToken eth = Utils.createEthereumBlockchainToken();
            String swapToAsset = "BAT";

            // Swap from
            String swapFromAssetSymbol = getIntent().getStringExtra("swapFromAssetSymbol");
            if (swapFromAssetSymbol == null
                    || swapFromAssetSymbol.equals(eth.symbol)) { // default swap from ETH
                updateBuySendSwapAsset(eth.symbol, eth, true);
            } else {
                mBlockchainRegistry.getTokenBySymbol(
                        BraveWalletConstants.MAINNET_CHAIN_ID, swapFromAssetSymbol, token -> {
                            if (token != null) {
                                updateBuySendSwapAsset(token.symbol, token, true);
                            }
                        });
            }

            if (mActivityType == ActivityType.SWAP) {
                // Swap to
                if (swapToAsset.equals(swapFromAssetSymbol)) { // swap from BAT
                    updateBuySendSwapAsset(eth.symbol, eth, false);
                } else {
                    mBlockchainRegistry.getTokenBySymbol(
                            BraveWalletConstants.MAINNET_CHAIN_ID, swapToAsset, token -> {
                                if (token != null) {
                                    updateBuySendSwapAsset(token.symbol, token, false);
                                }
                            });
                }
            }
        }
    }

    private void checkBalanceShowError(SwapResponse response, String errorResponse) {
        String value = mFromValueText.getText().toString();
        double valueFrom = 0;
        double gasLimit = 0;
        try {
            valueFrom = Double.parseDouble(value);
            gasLimit = Double.parseDouble(response.estimatedGas);
        } catch (NumberFormatException | NullPointerException ex) {
        }
        if (valueFrom > mConvertedFromBalance) {
            mBtnBuySendSwap.setText(getString(R.string.crypto_wallet_error_insufficient_balance));
            mBtnBuySendSwap.setEnabled(false);

            return;
        }

        final double fee = gasLimit * Utils.fromWei(response.gasPrice, 18);
        final double fromValue = valueFrom;
        assert mJsonRpcService != null;
        mJsonRpcService.getBalance(
                mCustomAccountAdapter.getTitleAtPosition(mAccountSpinner.getSelectedItemPosition()),
                CoinType.ETH, mCurrentChainId, (balance, error, errorMessage) -> {
                    warnWhenError(TAG, "getBalance", error, errorMessage);
                    if (error == ProviderError.SUCCESS) {
                        double currentBalance = Utils.fromHexWei(balance, 18);
                        boolean noCurrentToken = mCurrentBlockchainToken == null
                                || mCurrentBlockchainToken.contractAddress.isEmpty();

                        if (currentBalance < fee + (noCurrentToken ? fromValue : 0)) {
                            mBtnBuySendSwap.setText(
                                    getString(R.string.crypto_wallet_error_insufficient_gas));
                            mBtnBuySendSwap.setEnabled(false);

                            return;
                        }
                    }

                    if (errorResponse == null) {
                        mBtnBuySendSwap.setText(getString(R.string.swap));
                        mBtnBuySendSwap.setEnabled(true);
                        enableDisableSwapButton();
                        if (mBtnBuySendSwap.isEnabled() && mCurrentBlockchainToken != null
                                && mCurrentBlockchainToken.isErc20) {
                            // Check for ERC20 token allowance
                            checkAllowance(mCurrentBlockchainToken.contractAddress,
                                    response.allowanceTarget, fromValue);
                        }
                    } else {
                        if (Utils.isSwapLiquidityErrorReason(errorResponse)) {
                            mBtnBuySendSwap.setText(
                                    getString(R.string.crypto_wallet_error_insufficient_liquidity));
                        } else {
                            mBtnBuySendSwap.setText(
                                    getString(R.string.crypto_wallet_error_unknown_error));
                        }
                        mBtnBuySendSwap.setEnabled(false);
                    }
                });
    }

    private void checkAllowance(String contract, String spenderAddress, double amountToSend) {
        assert mJsonRpcService != null;
        assert mCurrentBlockchainToken != null;
        String ownerAddress =
                mCustomAccountAdapter.getTitleAtPosition(mAccountSpinner.getSelectedItemPosition());
        mJsonRpcService.getErc20TokenAllowance(
                contract, ownerAddress, spenderAddress, (allowance, error, errorMessage) -> {
                    warnWhenError(TAG, "getErc20TokenAllowance", error, errorMessage);
                    if (error != ProviderError.SUCCESS
                            || amountToSend <= Utils.fromHexWei(
                                       allowance, mCurrentBlockchainToken.decimals)) {
                        return;
                    }
                    mBtnBuySendSwap.setText(String.format(
                            getString(R.string.activate_erc20), mCurrentBlockchainToken.symbol));
                    mAllowanceTarget = spenderAddress;
                });
    }

    private void updateBalance(String address, boolean from) {
        assert mJsonRpcService != null;
        if (mJsonRpcService == null) {
            return;
        }
        BlockchainToken blockchainToken = mCurrentBlockchainToken;
        if (!from) {
            blockchainToken = mCurrentSwapToBlockchainToken;
        }
        if (blockchainToken == null || blockchainToken.contractAddress.isEmpty()) {
            mJsonRpcService.getBalance(
                    address, CoinType.ETH, mCurrentChainId, (balance, error, errorMessage) -> {
                        warnWhenError(TAG, "getBalance", error, errorMessage);
                        if (error != ProviderError.SUCCESS) {
                            return;
                        }
                        populateBalance(balance, from);
                    });
        } else {
            mJsonRpcService.getErc20TokenBalance(blockchainToken.contractAddress, address,
                    mCurrentChainId, (balance, error, errorMessage) -> {
                        warnWhenError(TAG, "getErc20TokenBalance", error, errorMessage);
                        if (error != ProviderError.SUCCESS) {
                            return;
                        }
                        populateBalance(balance, from);
                    });
        }
    }

    // We have to call that for SWAP, to update both from and to balance
    public void updateBalanceMaybeSwap(String address) {
        updateBalance(address, true);
        if (mActivityType == ActivityType.SWAP) {
            updateBalance(address, false);
        }
    }

    public String getCurrentSelectedAccountAddr() {
        return mCustomAccountAdapter.getTitleAtPosition(mAccountSpinner.getSelectedItemPosition());
    }

    private void populateBalance(String balance, boolean from) {
        BlockchainToken token = from ? mCurrentBlockchainToken : mCurrentSwapToBlockchainToken;
        TextView textView = from ? mFromBalanceText : mToBalanceText;

        int decimals = token != null ? token.decimals : 18;
        double fromToBalance = Utils.fromHexWei(balance, decimals);
        String text = getText(R.string.crypto_wallet_balance) + " "
                + String.format(Locale.getDefault(), "%.4f", fromToBalance);
        textView.setText(text);
        if (from) {
            mConvertedFromBalance = fromToBalance;
            if (mActivityType == ActivityType.SEND) mBtnBuySendSwap.setEnabled(fromToBalance != 0);
        } else
            mConvertedToBalance = fromToBalance;

        if (mActivityType == ActivityType.SWAP) {
            getSendSwapQuota(true, false);
        }
    }

    private int getIndexOf(Spinner spinner, String chainId, NetworkInfo[] customNetworks) {
        String strNetwork = Utils.getNetworkText(this, chainId, customNetworks).toString();
        for (int i = 0; i < spinner.getCount(); i++) {
            if (spinner.getItemAtPosition(i).toString().equalsIgnoreCase(strNetwork)) {
                return i;
            }
        }

        return 0;
    }

    private void setSendToFromValueValidationResult(
            String validationResult, boolean disableButtonOnError, boolean sendTo) {
        boolean validationSucceeded = (validationResult == null || validationResult.isEmpty());
        final TextView otherValueValidation = sendTo ? mFromSendValueValidation : mSendToValidation;
        final TextView thisValueValidation = sendTo ? mSendToValidation : mFromSendValueValidation;

        boolean otherValidationError = otherValueValidation.getVisibility() == View.VISIBLE;
        boolean buttonShouldBeEnabled = validationSucceeded || (sendTo && !disableButtonOnError);
        mBtnBuySendSwap.setEnabled(!otherValidationError && buttonShouldBeEnabled);

        if (validationSucceeded) {
            thisValueValidation.setText("");
            thisValueValidation.setVisibility(View.GONE);
        } else {
            thisValueValidation.setText(validationResult);
            thisValueValidation.setVisibility(View.VISIBLE);
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    private void adjustControls() {
        LinearLayout marketPriceSection = findViewById(R.id.market_price_section);
        LinearLayout toleranceSection = findViewById(R.id.tolerance_section);
        TextView currencySign = findViewById(R.id.currency_sign);
        TextView toEstimateText = findViewById(R.id.to_estimate_text);
        RadioGroup radioPerPercent = findViewById(R.id.per_percent_radiogroup);
        RadioGroup radioSlippageTolerance = findViewById(R.id.slippage_tolerance_radiogroup);
        EditText slippageValueText = findViewById(R.id.slippage_value_text);
        ImageView arrowDown = findViewById(R.id.arrow_down);
        radioPerPercent.clearCheck();

        // Common
        WalletCoinAdapter.AdapterType fromAdapterType = mActivityType == ActivityType.BUY
                ? WalletCoinAdapter.AdapterType.BUY_ASSETS_LIST
                : mActivityType == ActivityType.SEND
                        ? WalletCoinAdapter.AdapterType.SEND_ASSETS_LIST
                        : WalletCoinAdapter.AdapterType.SWAP_FROM_ASSETS_LIST;
        mFromAssetText.setOnClickListener(v -> {
            EditVisibleAssetsBottomSheetDialogFragment bottomSheetDialogFragment =
                    EditVisibleAssetsBottomSheetDialogFragment.newInstance(fromAdapterType);
            bottomSheetDialogFragment.setChainId(mCurrentChainId);
            bottomSheetDialogFragment.show(getSupportFragmentManager(),
                    EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
        });

        // Buy and Send
        if (mActivityType == ActivityType.BUY || mActivityType == ActivityType.SEND) {
            // radioBuySendSwap.setVisibility(View.GONE);
            marketPriceSection.setVisibility(View.GONE);
            arrowDown.setVisibility(View.GONE);
            toleranceSection.setVisibility(View.GONE);
        }

        // Individual
        if (mActivityType == ActivityType.BUY) {
            TextView fromBuyText = findViewById(R.id.from_buy_text);
            fromBuyText.setText(getText(R.string.buy_wallet));
            LinearLayout toSection = findViewById(R.id.to_section);
            toSection.setVisibility(View.GONE);
            mBtnBuySendSwap.setText(getText(R.string.buy_wallet));
            radioPerPercent.setVisibility(View.GONE);
        } else if (mActivityType == ActivityType.SEND) {
            currencySign.setVisibility(View.GONE);
            toEstimateText.setText(getText(R.string.to_address));
            mToValueText.setVisibility(View.GONE);
            mSendToAddrText = findViewById(R.id.send_to_addr_text);
            mSendToAddrText.setText("");
            mSendToAddrText.setHint(getText(R.string.to_address_edit));
            mSendToAddrText.addTextChangedListener(new FilterTextWatcherSendToAddr());
            mSendToAddrText.setOnFocusChangeListener(new OnFocusChangeListenerToSend());
            mFromValueText.addTextChangedListener(new FilterTextWatcherFromSendValue());

            mBtnBuySendSwap.setText(getText(R.string.send));
            LinearLayout toBalanceSection = findViewById(R.id.to_balance_section);
            toBalanceSection.setVisibility(View.GONE);

            mCameraSourcePreview = (CameraSourcePreview) findViewById(R.id.preview);
            ImageView qrCode = findViewById(R.id.qr_code);
            qrCode.setOnClickListener(v -> {
                RelativeLayout relativeLayout = findViewById(R.id.camera_layout);
                if (relativeLayout.getVisibility() == View.VISIBLE) {
                    if (null != mCameraSourcePreview) {
                        mCameraSourcePreview.stop();
                    }
                    relativeLayout.setVisibility(View.GONE);
                } else if (ensureCameraPermission()) {
                    qrCodeFunctionality();
                }
            });
        } else if (mActivityType == ActivityType.SWAP) {
            LinearLayout toSendSection = findViewById(R.id.send_to_section);
            toSendSection.setVisibility(View.GONE);
            currencySign.setVisibility(View.GONE);
            mToValueText.setText("");
            mToValueText.setHint("0");
            mToAssetText.setOnClickListener(v -> {
                EditVisibleAssetsBottomSheetDialogFragment bottomSheetDialogFragment =
                        EditVisibleAssetsBottomSheetDialogFragment.newInstance(
                                WalletCoinAdapter.AdapterType.SWAP_TO_ASSETS_LIST);
                bottomSheetDialogFragment.setChainId(mCurrentChainId);
                bottomSheetDialogFragment.show(getSupportFragmentManager(),
                        EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
            });
            enableDisableSwapButton();
            mSlippageToleranceText.setOnClickListener(v -> {
                LinearLayout toleranceSubSection = findViewById(R.id.tolerance_subsection);
                int visibility = toleranceSubSection.getVisibility();
                if (visibility == View.VISIBLE) {
                    toleranceSubSection.setVisibility(View.GONE);
                } else {
                    toleranceSubSection.setVisibility(View.VISIBLE);
                }
                for (int i = 0; i < radioSlippageTolerance.getChildCount(); i++) {
                    radioSlippageTolerance.getChildAt(i).setEnabled(visibility != View.VISIBLE);
                }
                findViewById(R.id.slippage_value_text).setEnabled(visibility != View.VISIBLE);
            });
            radioSlippageTolerance.setOnCheckedChangeListener(
                    new RadioGroup.OnCheckedChangeListener() {
                        @Override
                        public void onCheckedChanged(RadioGroup group, int checkedId) {
                            if (checkedId == -1) {
                                radioSlippageToleranceCheckedId = checkedId;
                                return;
                            }
                            double percent = 0d;
                            if (checkedId == R.id.slippage_per_05_radiobutton) {
                                percent = 0.5d;
                            } else if (checkedId == R.id.slippage_per_1_radiobutton) {
                                percent = 1d;
                            } else if (checkedId == R.id.slippage_per_2_radiobutton) {
                                percent = 2d;
                            }

                            if (!(radioSlippageToleranceCheckedId == 0
                                        || radioSlippageToleranceCheckedId == checkedId)) {
                                updateSlippagePercentage(percent);
                            }
                            if (radioSlippageToleranceCheckedId == -1) {
                                slippageValueText.setText("");
                            }
                            radioSlippageToleranceCheckedId = checkedId;
                        }
                    });
            slippageValueText.addTextChangedListener(new TextWatcher() {
                @Override
                public void afterTextChanged(Editable s) {}

                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    String inputPercent = s.toString().trim();
                    Double percent = 0d;
                    boolean hasError = false;
                    try {
                        percent = Double.parseDouble(inputPercent);
                        if (percent >= 100) hasError = true;
                    } catch (NumberFormatException ex) {
                        hasError = true;
                    }

                    if (!hasError && inputPercent.length() > 0) {
                        updateSlippagePercentage(percent);
                        radioSlippageTolerance.clearCheck();
                    }
                }
            });
            ImageView refreshPrice = findViewById(R.id.refresh_price);
            refreshPrice.setOnClickListener(v -> { getSendSwapQuota(true, false); });
            mFromValueText.addTextChangedListener(getTextWatcherFromToValueText(true));
            mToValueText.addTextChangedListener(getTextWatcherFromToValueText(false));
            findViewById(R.id.brave_fee).setVisibility(View.VISIBLE);
            TextView dexAggregator = findViewById(R.id.dex_aggregator);
            dexAggregator.setVisibility(View.VISIBLE);
            dexAggregator.setMovementMethod(LinkMovementMethod.getInstance());
            String dexAggregatorSrc = getString(R.string.swap_dex_aggregator_name);
            String degAggregatorText = getString(R.string.wallet_dex_aggregator, dexAggregatorSrc);

            NoUnderlineClickableSpan span = new NoUnderlineClickableSpan(
                    getResources(), R.color.brave_action_color, (textView) -> {
                        TabUtils.openUrlInNewTab(false, Utils.DEX_AGGREGATOR_URL);
                        TabUtils.bringChromeTabbedActivityToTheTop(BuySendSwapActivity.this);
                    });

            SpannableString dexAggregatorSpanStr = Utils.createSpannableString(
                    degAggregatorText, dexAggregatorSrc, span, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            dexAggregator.setText(dexAggregatorSpanStr);
            dexAggregator.setOnTouchListener((v, event) -> {
                if (event.getAction() == MotionEvent.ACTION_UP) {
                    // verify if the touch was on textview's drawable
                    // note: do not add right padding to this textview, or make sure to adjust the
                    // below condition for extra padding
                    if (event.getRawX() >= (v.getRight() - dexAggregator.getTotalPaddingRight())) {
                        Context context = BuySendSwapActivity.this;
                        Utils.showPopUp(context, context.getString(R.string.swap_text),
                                context.getString(R.string.brave_wallet_swap_disclaimer_description,
                                        getString(R.string.swap_dex_aggregator_name)),
                                context.getString(R.string.dialog_positive_button),
                                R.drawable.ic_info, (dialog, what) -> {});

                        return true;
                    }
                }
                return false;
            });
            resetSwapFromToAssets();
        }

        mBtnBuySendSwap.setOnClickListener(v -> {
            String from = mCustomAccountAdapter.getTitleAtPosition(
                    mAccountSpinner.getSelectedItemPosition());
            // TODO(sergz): Some kind of validation that we have enough balance
            String value = mFromValueText.getText().toString();
            if (mActivityType == ActivityType.SEND) {
                String to = mSendToAddrText.getText().toString();
                if (to.isEmpty()) {
                    return;
                }
                if (mCurrentBlockchainToken == null
                        || mCurrentBlockchainToken.contractAddress.isEmpty()) {
                    TxData data =
                            Utils.getTxData("", "", "", to, Utils.toHexWei(value, 18), new byte[0]);
                    sendTransaction(data, from, "", "");
                } else {
                    addUnapprovedTransactionERC20(to,
                            Utils.toHexWei(value, mCurrentBlockchainToken.decimals), from,
                            mCurrentBlockchainToken.contractAddress);
                }
            } else if (mActivityType == ActivityType.BUY) {
                if (mCurrentChainId.equals(BraveWalletConstants.MAINNET_CHAIN_ID)) {
                    assert mBlockchainRegistry != null;
                    String asset = mFromAssetText.getText().toString();
                    mBlockchainRegistry.getBuyUrl(
                            BraveWalletConstants.MAINNET_CHAIN_ID, from, asset, value, url -> {
                                TabUtils.openUrlInNewTab(false, url);
                                TabUtils.bringChromeTabbedActivityToTheTop(this);
                            });
                } else {
                    String url = getPerNetworkUiInfo(mCurrentChainId).linkUrl;
                    if (url != null && !url.isEmpty()) {
                        TabUtils.openUrlInNewTab(false, url);
                        TabUtils.bringChromeTabbedActivityToTheTop(this);
                    }
                }
            } else if (mActivityType == ActivityType.SWAP) {
                if (mCurrentBlockchainToken != null) {
                    String btnText = mBtnBuySendSwap.getText().toString();
                    String toCompare = String.format(
                            getString(R.string.activate_erc20), mCurrentBlockchainToken.symbol);
                    if (btnText.equals(toCompare)) {
                        activateErc20Allowance();

                        return;
                    }
                }
                mBtnBuySendSwap.setEnabled(false);
                getSendSwapQuota(true, true);
            }
        });

        radioPerPercent.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if (checkedId == -1) {
                    return;
                }
                double amountToGet = 0;
                if (checkedId == R.id.per_25_radiobutton) {
                    amountToGet = 0.25 * mConvertedFromBalance;
                } else if (checkedId == R.id.per_50_radiobutton) {
                    amountToGet = 0.5 * mConvertedFromBalance;
                } else if (checkedId == R.id.per_75_radiobutton) {
                    amountToGet = 0.75 * mConvertedFromBalance;
                } else {
                    amountToGet = mConvertedFromBalance;
                }

                mFromValueText.setText(String.format(Locale.getDefault(), "%f", amountToGet));
                radioPerPercent.clearCheck();
                if (mActivityType == ActivityType.SWAP) {
                    getSendSwapQuota(true, false);
                }
            }
        });
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mCameraSourcePreview != null) {
            mCameraSourcePreview.stop();
            RelativeLayout relativeLayout = findViewById(R.id.camera_layout);
            relativeLayout.setVisibility(View.GONE);
        }
    }

    private void updateSlippagePercentage(double percent) {
        SpannableString slippageToleranceSpannableText = new SpannableString(
                getString(R.string.crypto_wallet_tolerance_percentage, String.valueOf(percent)));
        mSlippageToleranceText.setText(slippageToleranceSpannableText);

        if (mActivityType == ActivityType.SWAP) {
            getSendSwapQuota(true, false);
        }
    }

    private void qrCodeFunctionality() {
        RelativeLayout relativeLayout = findViewById(R.id.camera_layout);
        if (relativeLayout.getVisibility() == View.VISIBLE) {
            return;
        }
        relativeLayout.setVisibility(View.VISIBLE);

        createCameraSource(true, false);
        try {
            startCameraSource();
        } catch (SecurityException exc) {
        }
    }

    private boolean ensureCameraPermission() {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                == PackageManager.PERMISSION_GRANTED) {
            return true;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[] {Manifest.permission.CAMERA}, RC_HANDLE_CAMERA_PERM);
        }

        return false;
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, String[] permissions, int[] grantResults) {
        if (requestCode != RC_HANDLE_CAMERA_PERM) {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);

            return;
        }

        if (grantResults.length != 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            // We have permission, so we can proceed with Camera
            qrCodeFunctionality();

            return;
        }
    }

    @SuppressLint("InlinedApi")
    private void createCameraSource(boolean autoFocus, boolean useFlash) {
        // A barcode detector is created to track barcodes.  An associated multi-processor instance
        // is set to receive the barcode detection results, track the barcodes, and maintain
        // graphics for each barcode on screen.  The factory is used by the multi-processor to
        // create a separate tracker instance for each barcode.
        BarcodeDetector barcodeDetector =
                new BarcodeDetector.Builder(this).setBarcodeFormats(Barcode.ALL_FORMATS).build();
        BarcodeTrackerFactory barcodeFactory = new BarcodeTrackerFactory(this);
        barcodeDetector.setProcessor(new MultiProcessor.Builder<>(barcodeFactory).build());

        if (!barcodeDetector.isOperational()) {
            // Note: The first time that an app using the barcode or face API is installed on a
            // device, GMS will download a native libraries to the device in order to do detection.
            // Usually this completes before the app is run for the first time.  But if that
            // download has not yet completed, then the above call will not detect any barcodes.
            //
            // isOperational() can be used to check if the required native libraries are currently
            // available.  The detectors will automatically become operational once the library
            // downloads complete on device.
            Log.w(TAG, "Detector dependencies are not yet available.");
        }

        // Creates and starts the camera.  Note that this uses a higher resolution in comparison
        // to other detection examples to enable the barcode detector to detect small barcodes
        // at long distances.
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);

        CameraSource.Builder builder =
                new CameraSource.Builder(this, barcodeDetector)
                        .setFacing(CameraSource.CAMERA_FACING_BACK)
                        .setRequestedPreviewSize(metrics.widthPixels, metrics.heightPixels)
                        .setRequestedFps(24.0f);

        // Make sure that auto focus is an available option
        builder = builder.setFocusMode(
                autoFocus ? Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE : null);

        mCameraSource =
                builder.setFlashMode(useFlash ? Camera.Parameters.FLASH_MODE_TORCH : null).build();
    }

    private void startCameraSource() throws SecurityException {
        if (mCameraSource != null && mCameraSourcePreview.mCameraExist) {
            // Check that the device has play services available.
            try {
                int code = GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(this);
                if (code != ConnectionResult.SUCCESS) {
                    Dialog dlg = GoogleApiAvailability.getInstance().getErrorDialog(
                            this, code, RC_HANDLE_GMS);
                    if (null != dlg) {
                        dlg.show();
                    }
                }
            } catch (ActivityNotFoundException e) {
                Log.e(TAG, "Unable to start camera source.", e);
                mCameraSource.release();
                mCameraSource = null;

                return;
            }
            try {
                mCameraSourcePreview.start(mCameraSource);
            } catch (IOException e) {
                Log.e(TAG, "Unable to start camera source.", e);
                mCameraSource.release();
                mCameraSource = null;
            }
        }
    }

    @Override
    public void onDetectedQrCode(Barcode barcode) {
        if (barcode == null) {
            return;
        }
        final String barcodeValue = barcode.displayValue;
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (null != mCameraSourcePreview) {
                    mCameraSourcePreview.stop();
                }
                RelativeLayout relativeLayout = findViewById(R.id.camera_layout);
                relativeLayout.setVisibility(View.GONE);
                mSendToAddrText.setText(barcodeValue);
            }
        });
    }

    @Override
    public void onBackPressed() {
        RelativeLayout relativeLayout = findViewById(R.id.camera_layout);
        if (relativeLayout.getVisibility() == View.VISIBLE) {
            if (null != mCameraSourcePreview) {
                mCameraSourcePreview.stop();
            }
            relativeLayout.setVisibility(View.GONE);

            return;
        }

        super.onBackPressed();
    }

    private TextWatcher getTextWatcherFromToValueText(boolean from) {
        return new TextWatcher() {
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                if (mFromValueText.hasFocus()) {
                    getSendSwapQuota(from, false);
                }
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void afterTextChanged(Editable s) {}
        };
    }

    private TextWatcher filterTextWatcherFrom = new TextWatcher() {
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            if (mFromValueText.hasFocus()) {
                getSendSwapQuota(true, false);
            }
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
    };

    private class FilterTextWatcherSendToAddr implements TextWatcher {
        Validations.SendToAccountAddress mValidator;

        public FilterTextWatcherSendToAddr() {
            mValidator = new Validations.SendToAccountAddress();
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            String fromAccountAddress = mCustomAccountAdapter.getTitleAtPosition(
                    mAccountSpinner.getSelectedItemPosition());

            mValidator.validate(mCurrentChainId, getKeyringService(), getBlockchainRegistry(),
                    getBraveWalletService(), fromAccountAddress, s.toString(),
                    (String validationResult, Boolean disableButton) -> {
                        setSendToFromValueValidationResult(validationResult, disableButton, true);
                    });
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
    }

    private class OnFocusChangeListenerToSend implements OnFocusChangeListener {
        Validations.SendToAccountAddress mValidator;

        public OnFocusChangeListenerToSend() {
            mValidator = new Validations.SendToAccountAddress();
        }

        @Override
        public void onFocusChange(View v, boolean hasFocus) {
            if (hasFocus) {
                String fromAccountAddress = mCustomAccountAdapter.getTitleAtPosition(
                        mAccountSpinner.getSelectedItemPosition());
                String receiverAccountAddress = ((EditText) v).getText().toString();

                mValidator.validate(mCurrentChainId, getKeyringService(), getBlockchainRegistry(),
                        getBraveWalletService(), fromAccountAddress, receiverAccountAddress,
                        (String validationResult, Boolean disableButton) -> {
                            setSendToFromValueValidationResult(
                                    validationResult, disableButton, true);
                        });
            } else {
                // Do not touch button
                mSendToValidation.setText("");
                mSendToValidation.setVisibility(View.GONE);
            }
        }
    }

    private class FilterTextWatcherFromSendValue implements TextWatcher {
        public FilterTextWatcherFromSendValue() {}

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            Double fromSendValue = 0d;
            try {
                fromSendValue = Double.parseDouble(s.toString());
            } catch (NumberFormatException ex) {
            }

            String validationResult = (fromSendValue > mConvertedFromBalance)
                    ? getString(R.string.crypto_wallet_error_insufficient_balance)
                    : "";

            setSendToFromValueValidationResult(validationResult, false, false);
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
    }

    private void activateErc20Allowance() {
        assert mAllowanceTarget != null && !mAllowanceTarget.isEmpty();
        assert mEthTxManagerProxy != null;
        assert mCurrentBlockchainToken != null;
        mEthTxManagerProxy.makeErc20ApproveData(mAllowanceTarget,
                Utils.toHexWei(String.format(Locale.getDefault(), "%.4f", mConvertedFromBalance),
                        mCurrentBlockchainToken.decimals),
                (success, data) -> {
                    if (!success) {
                        return;
                    }
                    TxData txData = Utils.getTxData(
                            "", "", "", mCurrentBlockchainToken.contractAddress, "0x0", data);
                    String from = mCustomAccountAdapter.getTitleAtPosition(
                            mAccountSpinner.getSelectedItemPosition());
                    sendTransaction(txData, from, "", "");
                });
    }

    private void sendTransaction(
            TxData data, String from, String maxPriorityFeePerGas, String maxFeePerGas) {
        assert mJsonRpcService != null;
        mJsonRpcService.getAllNetworks(CoinType.ETH, networks -> {
            boolean isEIP1559 = false;
            // We have hardcoded EIP-1559 gas fields.
            if (!maxPriorityFeePerGas.isEmpty() && !maxFeePerGas.isEmpty()) {
                isEIP1559 = true;
            } else if (!data.gasPrice.isEmpty()) {
                // We have hardcoded legacy tx gas fields.
                isEIP1559 = false;
            }
            for (NetworkInfo network : networks) {
                if (!mCurrentChainId.equals(network.chainId)) {
                    continue;
                }
                isEIP1559 = network.data.getEthData().isEip1559;
            }

            assert mTxService != null;
            if (isEIP1559) {
                TxData1559 txData1559 = new TxData1559();
                txData1559.baseData = data;
                txData1559.chainId = mCurrentChainId;
                txData1559.maxPriorityFeePerGas = maxPriorityFeePerGas;
                txData1559.maxFeePerGas = maxFeePerGas;
                txData1559.gasEstimation = new GasEstimation1559();
                txData1559.gasEstimation.slowMaxPriorityFeePerGas = "";
                txData1559.gasEstimation.slowMaxFeePerGas = "";
                txData1559.gasEstimation.avgMaxPriorityFeePerGas = "";
                txData1559.gasEstimation.avgMaxFeePerGas = "";
                txData1559.gasEstimation.fastMaxPriorityFeePerGas = "";
                txData1559.gasEstimation.fastMaxFeePerGas = "";
                txData1559.gasEstimation.baseFeePerGas = "";
                TxDataUnion txDataUnion = new TxDataUnion();
                txDataUnion.setEthTxData1559(txData1559);
                mTxService.addUnapprovedTransaction(
                        txDataUnion, from, (success, tx_meta_id, error_message) -> {
                            // Do nothing here when success as we will receive an
                            // unapproved transaction in TxServiceObserver.
                            // When we have error, let the user know,
                            // error_message is localized, do not disable send button
                            setSendToFromValueValidationResult(error_message, false, true);
                        });
            } else {
                TxDataUnion txDataUnion = new TxDataUnion();
                txDataUnion.setEthTxData(data);
                mTxService.addUnapprovedTransaction(
                        txDataUnion, from, (success, tx_meta_id, error_message) -> {
                            // Do nothing here when success as we will receive an
                            // unapproved transaction in TxServiceObserver.
                            // When we have error, let the user know,
                            // error_message is localized, do not disable send button
                            setSendToFromValueValidationResult(error_message, false, true);
                        });
            }
        });
    }

    private void addUnapprovedTransactionERC20(
            String to, String value, String from, String contractAddress) {
        assert mEthTxManagerProxy != null;
        if (mEthTxManagerProxy == null) {
            return;
        }
        mEthTxManagerProxy.makeErc20TransferData(to, value, (success, data) -> {
            if (!success) {
                return;
            }
            TxData txData = Utils.getTxData("", "", "", contractAddress, "0x0", data);
            sendTransaction(txData, from, "", "");
        });
    }

    public void showApproveTransactionDialog(TransactionInfo txInfo) {
        String accountName =
                mCustomAccountAdapter.getNameAtPosition(mAccountSpinner.getSelectedItemPosition());
        if (mActivityType == ActivityType.SWAP) {
            mBtnBuySendSwap.setEnabled(true);
            if (mCurrentBlockchainToken != null) {
                String btnText = mBtnBuySendSwap.getText().toString();
                String toCompare = String.format(
                        getString(R.string.activate_erc20), mCurrentBlockchainToken.symbol);
                if (btnText.equals(toCompare)) {
                    mActivateAllowanceTxId = txInfo.id;
                }
            }
        }
        ApproveTxBottomSheetDialogFragment approveTxBottomSheetDialogFragment =
                ApproveTxBottomSheetDialogFragment.newInstance(txInfo, accountName);
        approveTxBottomSheetDialogFragment.setApprovedTxObserver(this);
        approveTxBottomSheetDialogFragment.show(
                getSupportFragmentManager(), ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
    }

    @Override
    public void OnTxApprovedRejected(boolean approved, String accountName, String txId) {
        if (approved) {
            finish();
        }
    }

    @Override
    public void OnTxPending(String accountName, String txId) {}

    public void showSwapButtonText() {
        mBtnBuySendSwap.setText(getString(R.string.swap));
    }

    public void updateBuySendSwapAsset(
            String asset, BlockchainToken blockchainToken, boolean buySend) {
        if (!buySend && mCurrentBlockchainToken != null
                        && mCurrentBlockchainToken.symbol.equals(blockchainToken.symbol)
                || buySend && mCurrentSwapToBlockchainToken != null
                        && mCurrentSwapToBlockchainToken.symbol.equals(blockchainToken.symbol))
            return;

        TextView assetText = buySend ? mFromAssetText : mToAssetText;
        assetText.setText(asset);
        if (buySend)
            mCurrentBlockchainToken = blockchainToken;
        else
            mCurrentSwapToBlockchainToken = blockchainToken;

        BlockchainToken token = buySend ? mCurrentBlockchainToken : mCurrentSwapToBlockchainToken;
        // Replace USDC and DAI contract addresses for Ropsten network
        token.contractAddress =
                Utils.getContractAddress(mCurrentChainId, token.symbol, token.contractAddress);
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();
        if (token.symbol.equals("ETH")) {
            token.logo = "eth.png";
        }
        String iconPath =
                blockchainToken.logo.isEmpty() ? null : ("file://" + tokensPath + "/" + token.logo);
        if (!token.logo.isEmpty()) {
            Utils.setBitmapResource(mExecutor, mHandler, this, iconPath, R.drawable.ic_eth_24, null,
                    assetText, true);
        } else {
            Utils.setBlockiesBitmapCustomAsset(mExecutor, mHandler, null, token.contractAddress,
                    token.symbol, getResources().getDisplayMetrics().density, assetText, this, true,
                    (float) 0.5);
        }
        if (buySend && mActivityType == ActivityType.SWAP || !buySend) {
            enableDisableSwapButton();
            getSendSwapQuota(true, false);
        }
    }

    private void enableDisableSwapButton() {
        boolean swapToTokenNullOrEmpty = mCurrentSwapToBlockchainToken == null
                || mCurrentSwapToBlockchainToken.contractAddress.isEmpty();
        boolean tokenNullOrEmpty = mCurrentBlockchainToken == null
                || mCurrentBlockchainToken.contractAddress.isEmpty();
        boolean disable = swapToTokenNullOrEmpty && tokenNullOrEmpty
                || mCurrentSwapToBlockchainToken.contractAddress.equals(
                        mCurrentBlockchainToken.contractAddress);

        mBtnBuySendSwap.setEnabled(!disable);
    }

    @Override
    public void onConnectionError(MojoException e) {
        super.onConnectionError(e);
        mSwapService.close();

        mSwapService = null;
        InitSwapService();
    }

    private void InitSwapService() {
        if (mSwapService != null) {
            return;
        }

        mSwapService = SwapServiceFactory.getInstance().getSwapService(this);
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        InitSwapService();

        if (mJsonRpcService != null) {
            mJsonRpcService.getChainId(CoinType.ETH, chainId -> {
                mCurrentChainId = chainId;
                Spinner spinner = findViewById(R.id.network_spinner);
                spinner.setOnItemSelectedListener(this);
                mJsonRpcService.getAllNetworks(CoinType.ETH, chains -> {
                    NetworkInfo[] customNetworks = new NetworkInfo[0];
                    // We want to hide custom networks for BUY and SWAP screens. We are
                    // going to add a support for SWAP at least in the near future.
                    if (mActivityType == ActivityType.SEND) {
                        customNetworks = Utils.getCustomNetworks(chains);
                    }
                    // Creating adapter for spinner
                    NetworkSpinnerAdapter dataAdapter = new NetworkSpinnerAdapter(this,
                            Utils.getNetworksList(this, customNetworks),
                            Utils.getNetworksAbbrevList(this, customNetworks));
                    spinner.setAdapter(dataAdapter);
                    spinner.setSelection(getIndexOf(spinner, chainId, customNetworks));

                    for (NetworkInfo chain : chains) {
                        if (chainId.equals(chain.chainId)) {
                            TextView fromAssetText = findViewById(R.id.from_asset_text);
                            TextView marketLimitPriceText =
                                    findViewById(R.id.market_limit_price_text);
                            if (Utils.isCustomNetwork(chainId)) {
                                Utils.setBlockiesBitmapCustomAsset(mExecutor, mHandler, null, "",
                                        chain.symbol, getResources().getDisplayMetrics().density,
                                        fromAssetText, this, true, (float) 0.5);
                            }
                            fromAssetText.setText(chain.symbol);
                            marketLimitPriceText.setText(String.format(
                                    getString(R.string.market_price_in), chain.symbol));
                            break;
                        }
                    }
                });
            });
        }
        if (mKeyringService != null) {
            mKeyringService.getKeyringInfo(BraveWalletConstants.DEFAULT_KEYRING_ID, keyring -> {
                String[] accountNames = new String[keyring.accountInfos.length];
                String[] accountTitles = new String[keyring.accountInfos.length];
                int currentPos = 0;
                for (AccountInfo info : keyring.accountInfos) {
                    accountNames[currentPos] = info.name;
                    accountTitles[currentPos] = info.address;
                    currentPos++;
                }
                mCustomAccountAdapter = new AccountSpinnerAdapter(
                        getApplicationContext(), accountNames, accountTitles);
                mAccountSpinner.setAdapter(mCustomAccountAdapter);
                mAccountSpinner.setOnItemSelectedListener(this);
                if (accountTitles.length > 0) {
                    updateBalanceMaybeSwap(accountTitles[0]);
                }

                // updateBuySendSwapAsset needs mCustomAccountAdapter to be initialized
                resetSwapFromToAssets();
            });
        }
    }

    @Override
    public void onNewUnapprovedTx(TransactionInfo txInfo) {
        showApproveTransactionDialog(txInfo);
    }

    @Override
    public void onTransactionStatusChanged(TransactionInfo txInfo) {
        if (txInfo.id.equals(mActivateAllowanceTxId)
                && txInfo.txStatus == TransactionStatus.SUBMITTED) {
            mActivateAllowanceTxId = "";
            showSwapButtonText();
        }
    }
}
