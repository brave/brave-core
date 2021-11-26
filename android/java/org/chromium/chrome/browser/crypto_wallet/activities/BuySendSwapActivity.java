/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.DisplayMetrics;
import android.view.MenuItem;
import android.view.View;
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
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.brave_wallet.mojom.BraveWalletConstants;
import org.chromium.brave_wallet.mojom.ErcToken;
import org.chromium.brave_wallet.mojom.ErcTokenRegistry;
import org.chromium.brave_wallet.mojom.EthJsonRpcController;
import org.chromium.brave_wallet.mojom.EthTxController;
import org.chromium.brave_wallet.mojom.EthTxControllerObserver;
import org.chromium.brave_wallet.mojom.EthereumChain;
import org.chromium.brave_wallet.mojom.GasEstimation1559;
import org.chromium.brave_wallet.mojom.KeyringController;
import org.chromium.brave_wallet.mojom.KeyringInfo;
import org.chromium.brave_wallet.mojom.SwapController;
import org.chromium.brave_wallet.mojom.SwapParams;
import org.chromium.brave_wallet.mojom.SwapResponse;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionStatus;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.AssetRatioControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.ERCTokenRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.EthJsonRpcControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.EthTxControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.KeyringControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.SwapControllerFactory;
import org.chromium.chrome.browser.crypto_wallet.adapters.AccountSpinnerAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSpinnerAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.ApproveTxBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.EditVisibleAssetsBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.init.AsyncInitializationActivity;
import org.chromium.chrome.browser.qrreader.BarcodeTracker;
import org.chromium.chrome.browser.qrreader.BarcodeTrackerFactory;
import org.chromium.chrome.browser.qrreader.CameraSource;
import org.chromium.chrome.browser.qrreader.CameraSourcePreview;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BuySendSwapActivity extends AsyncInitializationActivity
        implements ConnectionErrorHandler, AdapterView.OnItemSelectedListener,
                   BarcodeTracker.BarcodeGraphicTrackerCallback {
    private final static String TAG = "BuySendSwapActivity";
    private final static String ETHEREUM_CONTRACT_FOR_SWAP =
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    private static final int RC_HANDLE_CAMERA_PERM = 113;
    // Intent request code to handle updating play services if needed.
    private static final int RC_HANDLE_GMS = 9001;

    private CameraSource mCameraSource;
    private CameraSourcePreview mCameraSourcePreview;

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

    private class EthTxControllerObserverImpl implements EthTxControllerObserver {
        private BuySendSwapActivity mParentActivity;

        public EthTxControllerObserverImpl(BuySendSwapActivity parentActivity) {
            mParentActivity = parentActivity;
        }

        @Override
        public void onNewUnapprovedTx(TransactionInfo txInfo) {
            assert mParentActivity != null;
            mParentActivity.showApproveTransactionDialog(txInfo);
        }

        @Override
        public void onTransactionStatusChanged(TransactionInfo txInfo) {
            if (txInfo.id.equals(mParentActivity.mActivateAllowanceTxId)
                    && txInfo.txStatus == TransactionStatus.SUBMITTED) {
                mParentActivity.mActivateAllowanceTxId = "";
                mParentActivity.showSwapButtonText();
            }
        }

        @Override
        public void onUnapprovedTxUpdated(TransactionInfo txInfo) {}

        @Override
        public void close() {}

        @Override
        public void onConnectionError(MojoException e) {}
    }

    public String mActivateAllowanceTxId;

    private ErcTokenRegistry mErcTokenRegistry;
    private EthJsonRpcController mEthJsonRpcController;
    private EthTxController mEthTxController;
    private KeyringController mKeyringController;
    private ActivityType mActivityType;
    private AccountSpinnerAdapter mCustomAccountAdapter;
    private double mConvertedFromBalance;
    private double mConvertedToBalance;
    private EthTxControllerObserverImpl mEthTxControllerObserver;
    private AssetRatioController mAssetRatioController;
    private ErcToken mCurrentErcToken;
    private ErcToken mCurrentSwapToErcToken;
    private SwapController mSwapController;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private String mCurrentChainId;
    private String mAllowanceTarget;
    private Spinner mAccountSpinner;

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mCameraSourcePreview != null) {
            mCameraSourcePreview.release();
        }
        mKeyringController.close();
        mAssetRatioController.close();
        mErcTokenRegistry.close();
        mEthJsonRpcController.close();
        mEthTxController.close();
        mSwapController.close();
    }

    @Override
    @SuppressLint("SetTextI18n")
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_buy_send_swap);

        mActivateAllowanceTxId = "";
        Intent intent = getIntent();
        mActivityType = ActivityType.valueOf(
                intent.getIntExtra("activityType", ActivityType.BUY.getValue()));

        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        EditText fromValueText = findViewById(R.id.from_value_text);
        fromValueText.setText("");
        fromValueText.setHint("0");

        TextView fromBalanceText = findViewById(R.id.from_balance_text);
        TextView fromAssetText = findViewById(R.id.from_asset_text);
        fromAssetText.setText("ETH");

        TextView toBalanceText = findViewById(R.id.to_balance_text);
        TextView toAssetText = findViewById(R.id.to_asset_text);
        toAssetText.setText("ETH");

        TextView marketPriceValueText = findViewById(R.id.market_price_value_text);
        Spinner spinner = findViewById(R.id.network_spinner);
        spinner.setOnItemSelectedListener(this);
        // Creating adapter for spinner
        NetworkSpinnerAdapter dataAdapter = new NetworkSpinnerAdapter(
                this, Utils.getNetworksList(this), Utils.getNetworksAbbrevList(this));
        spinner.setAdapter(dataAdapter);

        onInitialLayoutInflationComplete();

        adjustControls();
        InitErcTokenRegistry();
        InitEthJsonRpcController();
        InitEthTxController();
        InitKeyringController();
        InitAssetRatioController();
        InitSwapController();

        if (mEthJsonRpcController != null) {
            mEthJsonRpcController.getChainId(chainId -> {
                mCurrentChainId = chainId;
                spinner.setSelection(getIndexOf(spinner, chainId));
            });
        }
        if (mKeyringController != null) {
            mKeyringController.getDefaultKeyringInfo(keyring -> {
                String[] accountNames = new String[keyring.accountInfos.length];
                String[] accountTitles = new String[keyring.accountInfos.length];
                int currentPos = 0;
                for (AccountInfo info : keyring.accountInfos) {
                    accountNames[currentPos] = info.name;
                    accountTitles[currentPos] = info.address;
                    currentPos++;
                }
                mAccountSpinner = findViewById(R.id.accounts_spinner);
                mCustomAccountAdapter = new AccountSpinnerAdapter(
                        getApplicationContext(), accountNames, accountTitles);
                mAccountSpinner.setAdapter(mCustomAccountAdapter);
                mAccountSpinner.setOnItemSelectedListener(this);
                if (accountTitles.length > 0) {
                    updateBalance(accountTitles[0], true);
                    if (mActivityType == ActivityType.SWAP) {
                        updateBalance(accountTitles[0], false);
                    }
                }
            });
        }
    }

    private class BuySendSwapUiInfo {
        public boolean shouldShowBuyControls;
        public String titleText;
        public String secondText;
        public String buttonText;
        public String linkUrl;
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
        ((Button) findViewById(R.id.btn_buy_send_swap)).setText(buySendSwapUiInfo.buttonText);
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        if (parent.getId() == R.id.network_spinner) {
            String item = parent.getItemAtPosition(position).toString();
            final String chainId = Utils.getNetworkConst(this, item);

            if (mActivityType == ActivityType.BUY) {
                adjustTestFaucetControls(getPerNetworkUiInfo(chainId));
            }

            if (mEthJsonRpcController != null) {
                mEthJsonRpcController.setNetwork(chainId, (success) -> {
                    if (!success) {
                        Log.e(TAG, "Could not set network");
                    }
                    mCurrentChainId = chainId;
                });
            }
            updateBalance(mCustomAccountAdapter.getTitleAtPosition(
                                  mAccountSpinner.getSelectedItemPosition()),
                    true);
            if (mActivityType == ActivityType.SWAP) {
                updateBalance(mCustomAccountAdapter.getTitleAtPosition(
                                      mAccountSpinner.getSelectedItemPosition()),
                        false);
            }
        } else if (parent.getId() == R.id.accounts_spinner) {
            updateBalance(mCustomAccountAdapter.getTitleAtPosition(position), true);
            if (mActivityType == ActivityType.SWAP) {
                updateBalance(mCustomAccountAdapter.getTitleAtPosition(position), false);
            }
        } else if (parent.getId() == R.id.slippage_tolerance_spinner
                && mActivityType == ActivityType.SWAP) {
            getSendSwapQuota(true, false);
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> arg0) {}

    private void getSendSwapQuota(boolean calculatePerSellAsset, boolean sendTx) {
        String from =
                mCustomAccountAdapter.getTitleAtPosition(mAccountSpinner.getSelectedItemPosition());
        EditText fromValueText = findViewById(R.id.from_value_text);
        String value = fromValueText.getText().toString();
        EditText toValueText = findViewById(R.id.to_value_text);
        String valueTo = toValueText.getText().toString();
        String buyAddress = ETHEREUM_CONTRACT_FOR_SWAP;
        int decimalsTo = 18;
        if (mCurrentSwapToErcToken != null) {
            decimalsTo = mCurrentSwapToErcToken.decimals;
            buyAddress = mCurrentSwapToErcToken.contractAddress;
            if (buyAddress.isEmpty()) {
                buyAddress = ETHEREUM_CONTRACT_FOR_SWAP;
            }
        }
        String sellAddress = ETHEREUM_CONTRACT_FOR_SWAP;
        int decimalsFrom = 18;
        if (mCurrentErcToken != null) {
            decimalsFrom = mCurrentErcToken.decimals;
            sellAddress = mCurrentErcToken.contractAddress;
            if (sellAddress.isEmpty()) {
                sellAddress = ETHEREUM_CONTRACT_FOR_SWAP;
            }
        }
        Spinner slippageToleranceSpinner = findViewById(R.id.slippage_tolerance_spinner);
        String percent =
                slippageToleranceSpinner
                        .getItemAtPosition(slippageToleranceSpinner.getSelectedItemPosition())
                        .toString();
        percent = percent.replace("%", "");

        SwapParams swapParams = new SwapParams();
        swapParams.takerAddress = from;
        swapParams.sellAmount = Utils.toWei(value, decimalsFrom);
        if (swapParams.sellAmount.equals("0") || !calculatePerSellAsset) {
            swapParams.sellAmount = "";
        }
        swapParams.buyAmount = Utils.toWei(valueTo, decimalsTo);
        if (swapParams.buyAmount.equals("0") || calculatePerSellAsset) {
            swapParams.buyAmount = "";
        }
        swapParams.buyToken = buyAddress;
        swapParams.sellToken = sellAddress;
        try {
            swapParams.slippagePercentage = Double.parseDouble(percent) / 100;
        } catch (NumberFormatException ex) {
        }
        swapParams.gasPrice = "";

        assert mSwapController != null;
        if (!sendTx) {
            mSwapController.getPriceQuote(swapParams, (success, response, error_response) -> {
                workWithSwapQuota(
                        success, response, error_response, calculatePerSellAsset, sendTx, from);
            });
        } else {
            mSwapController.getTransactionPayload(
                    swapParams, (success, response, error_response) -> {
                        workWithSwapQuota(success, response, error_response, calculatePerSellAsset,
                                sendTx, from);
                    });
        }
    }

    private void workWithSwapQuota(boolean success, SwapResponse response, String errorResponse,
            boolean calculatePerSellAsset, boolean sendTx, String from) {
        if (!success) {
            SwapResponse nullResponse = new SwapResponse();
            nullResponse.sellAmount = "0";
            nullResponse.buyAmount = "0";
            nullResponse.price = "0";
            updateSwapControls(nullResponse, calculatePerSellAsset, errorResponse);
            if (errorResponse != null) {
                if (sendTx) {
                    Log.e(TAG, "Swap error: " + errorResponse);
                }
            }

            return;
        }
        updateSwapControls(response, calculatePerSellAsset, null);
        if (sendTx) {
            TxData data = Utils.getTxData("", Utils.toWeiHex(response.gasPrice),
                    Utils.toWeiHex(response.estimatedGas), response.to,
                    Utils.toWeiHex(response.value), Utils.hexStrToNumberArray(response.data));
            sendSwapTransaction(data, from);
        }
    }

    private void sendSwapTransaction(TxData data, String from) {
        assert mAssetRatioController != null;
        mAssetRatioController.getGasOracle(estimation -> {
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
        EditText fromValueText = findViewById(R.id.from_value_text);
        EditText toValueText = findViewById(R.id.to_value_text);
        TextView marketPriceValueText = findViewById(R.id.market_price_value_text);
        if (!calculatePerSellAsset) {
            int decimals = 18;
            if (mCurrentErcToken != null) {
                decimals = mCurrentErcToken.decimals;
            }
            fromValueText.setText(String.format(
                    Locale.getDefault(), "%.4f", Utils.fromWei(response.sellAmount, decimals)));
        } else {
            int decimals = 18;
            if (mCurrentSwapToErcToken != null) {
                decimals = mCurrentSwapToErcToken.decimals;
            }
            toValueText.setText(String.format(
                    Locale.getDefault(), "%.4f", Utils.fromWei(response.buyAmount, decimals)));
        }
        if (calculatePerSellAsset) {
            marketPriceValueText.setText(response.price);
        } else {
            try {
                double price = Double.parseDouble(response.price);
                if (price != 0) {
                    price = 1 / price;
                }
                marketPriceValueText.setText(String.format(Locale.getDefault(), "%.18f", price));
            } catch (NumberFormatException | NullPointerException ex) {
            }
        }
        TextView marketLimitPriceText = findViewById(R.id.market_limit_price_text);
        String symbol = "ETH";
        if (mCurrentErcToken != null) {
            symbol = mCurrentErcToken.symbol;
        }
        marketLimitPriceText.setText(String.format(getString(R.string.market_price_in), symbol));
        checkBalanceShowError(response, errorResponse);
    }

    private void checkBalanceShowError(SwapResponse response, String errorResponse) {
        final Button btnBuySendSwap = findViewById(R.id.btn_buy_send_swap);
        EditText fromValueText = findViewById(R.id.from_value_text);
        String value = fromValueText.getText().toString();
        double valueFrom = 0;
        double gasLimit = 0;
        try {
            valueFrom = Double.parseDouble(value);
            gasLimit = Double.parseDouble(response.estimatedGas);
        } catch (NumberFormatException | NullPointerException ex) {
        }
        if (valueFrom > mConvertedFromBalance) {
            btnBuySendSwap.setText(getString(R.string.crypto_wallet_error_insufficient_balance));
            btnBuySendSwap.setEnabled(false);

            return;
        }
        final double fee = gasLimit * Utils.fromWei(response.gasPrice, 18);
        final double fromValue = valueFrom;
        assert mEthJsonRpcController != null;
        mEthJsonRpcController.getBalance(
                mCustomAccountAdapter.getTitleAtPosition(mAccountSpinner.getSelectedItemPosition()),
                (success, balance) -> {
                    if (success) {
                        double currentBalance = Utils.fromHexWei(balance, 18);
                        if (mCurrentErcToken == null
                                || mCurrentErcToken.contractAddress.isEmpty()) {
                            if (currentBalance < fee + fromValue) {
                                btnBuySendSwap.setText(
                                        getString(R.string.crypto_wallet_error_insufficient_gas));
                                btnBuySendSwap.setEnabled(false);

                                return;
                            }
                        } else {
                            if (currentBalance < fee) {
                                btnBuySendSwap.setText(
                                        getString(R.string.crypto_wallet_error_insufficient_gas));
                                btnBuySendSwap.setEnabled(false);

                                return;
                            }
                        }
                    }

                    if (errorResponse == null) {
                        btnBuySendSwap.setText(getString(R.string.swap));
                        btnBuySendSwap.setEnabled(true);
                        enableDisableSwapButton();
                        if (btnBuySendSwap.isEnabled() && mCurrentErcToken != null
                                && mCurrentErcToken.isErc20) {
                            // Check for ERC20 token allowance
                            checkAllowance(mCurrentErcToken.contractAddress,
                                    response.allowanceTarget, fromValue);
                        }
                    } else {
                        if (Utils.isSwapLiquidityErrorReason(errorResponse)) {
                            btnBuySendSwap.setText(
                                    getString(R.string.crypto_wallet_error_insufficient_liquidity));
                        } else {
                            btnBuySendSwap.setText(
                                    getString(R.string.crypto_wallet_error_unknown_error));
                        }
                        btnBuySendSwap.setEnabled(false);
                    }
                });
    }

    private void checkAllowance(String contract, String spenderAddress, double amountToSend) {
        assert mEthJsonRpcController != null;
        assert mCurrentErcToken != null;
        String ownerAddress =
                mCustomAccountAdapter.getTitleAtPosition(mAccountSpinner.getSelectedItemPosition());
        mEthJsonRpcController.getErc20TokenAllowance(
                contract, ownerAddress, spenderAddress, (success, allowance) -> {
                    if (!success
                            || amountToSend
                                    <= Utils.fromHexWei(allowance, mCurrentErcToken.decimals)) {
                        return;
                    }
                    Button btnBuySendSwap = findViewById(R.id.btn_buy_send_swap);
                    btnBuySendSwap.setText(String.format(
                            getString(R.string.activate_erc20), mCurrentErcToken.symbol));
                    mAllowanceTarget = spenderAddress;
                });
    }

    private void updateBalance(String address, boolean from) {
        assert mEthJsonRpcController != null;
        if (mEthJsonRpcController == null) {
            return;
        }
        ErcToken ercToken = mCurrentErcToken;
        if (!from) {
            ercToken = mCurrentSwapToErcToken;
        }
        if (ercToken == null || ercToken.contractAddress.isEmpty()) {
            mEthJsonRpcController.getBalance(address, (success, balance) -> {
                if (!success) {
                    return;
                }
                populateBalance(balance, from);
            });
        } else {
            mEthJsonRpcController.getErc20TokenBalance(
                    ercToken.contractAddress, address, (success, balance) -> {
                        if (!success) {
                            return;
                        }
                        populateBalance(balance, from);
                    });
        }
    }

    private void populateBalance(String balance, boolean from) {
        int decimals = 18;
        if (from) {
            TextView fromBalanceText = findViewById(R.id.from_balance_text);
            if (mCurrentErcToken != null) {
                decimals = mCurrentErcToken.decimals;
            }
            mConvertedFromBalance = Utils.fromHexWei(balance, decimals);
            String text = getText(R.string.crypto_wallet_balance) + " "
                    + String.format(Locale.getDefault(), "%.4f", mConvertedFromBalance);
            fromBalanceText.setText(text);
        } else {
            TextView toBalanceText = findViewById(R.id.to_balance_text);
            if (mCurrentSwapToErcToken != null) {
                decimals = mCurrentSwapToErcToken.decimals;
            }
            mConvertedToBalance = Utils.fromHexWei(balance, decimals);
            String text = getText(R.string.crypto_wallet_balance) + " "
                    + String.format(Locale.getDefault(), "%.4f", mConvertedToBalance);
            toBalanceText.setText(text);
        }
        if (mActivityType == ActivityType.SWAP) {
            getSendSwapQuota(true, false);
        }
    }

    private int getIndexOf(Spinner spinner, String chainId) {
        String strNetwork = Utils.getNetworkText(this, chainId).toString();
        for (int i = 0; i < spinner.getCount(); i++) {
            if (spinner.getItemAtPosition(i).toString().equalsIgnoreCase(strNetwork)) {
                return i;
            }
        }

        return 0;
    }

    private void adjustControls() {
        EditText toValueText = findViewById(R.id.to_value_text);
        TextView marketPriceValueText = findViewById(R.id.market_price_value_text);
        // RadioGroup radioBuySendSwap = findViewById(R.id.buy_send_swap_type_radio_group);
        LinearLayout marketPriceSection = findViewById(R.id.market_price_section);
        LinearLayout toleranceSection = findViewById(R.id.tolerance_section);
        Button btnBuySendSwap = findViewById(R.id.btn_buy_send_swap);
        TextView currencySign = findViewById(R.id.currency_sign);
        TextView toEstimateText = findViewById(R.id.to_estimate_text);
        TextView assetFromDropDown = findViewById(R.id.from_asset_text);
        RadioGroup radioPerPercent = findViewById(R.id.per_percent_radiogroup);
        ImageView arrowDown = findViewById(R.id.arrow_down);
        radioPerPercent.clearCheck();
        if (mActivityType == ActivityType.BUY) {
            TextView fromBuyText = findViewById(R.id.from_buy_text);
            fromBuyText.setText(getText(R.string.buy_wallet));
            LinearLayout toSection = findViewById(R.id.to_section);
            toSection.setVisibility(View.GONE);
            // radioBuySendSwap.setVisibility(View.GONE);
            marketPriceSection.setVisibility(View.GONE);
            arrowDown.setVisibility(View.GONE);
            toleranceSection.setVisibility(View.GONE);
            btnBuySendSwap.setText(getText(R.string.buy_wallet));
            radioPerPercent.setVisibility(View.GONE);
            assetFromDropDown.setOnClickListener(v -> {
                EditVisibleAssetsBottomSheetDialogFragment bottomSheetDialogFragment =
                        EditVisibleAssetsBottomSheetDialogFragment.newInstance(
                                WalletCoinAdapter.AdapterType.BUY_ASSETS_LIST);
                bottomSheetDialogFragment.show(getSupportFragmentManager(),
                        EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
            });
        } else if (mActivityType == ActivityType.SEND) {
            currencySign.setVisibility(View.GONE);
            toEstimateText.setText(getText(R.string.to_address));
            toValueText.setVisibility(View.GONE);
            EditText toSendValueText = findViewById(R.id.to_send_value_text);
            toSendValueText.setText("");
            toSendValueText.setHint(getText(R.string.to_address_edit));
            arrowDown.setVisibility(View.GONE);
            // radioBuySendSwap.setVisibility(View.GONE);
            marketPriceSection.setVisibility(View.GONE);
            toleranceSection.setVisibility(View.GONE);
            btnBuySendSwap.setText(getText(R.string.send));
            LinearLayout toBalanceSection = findViewById(R.id.to_balance_section);
            toBalanceSection.setVisibility(View.GONE);
            assetFromDropDown.setOnClickListener(v -> {
                EditVisibleAssetsBottomSheetDialogFragment bottomSheetDialogFragment =
                        EditVisibleAssetsBottomSheetDialogFragment.newInstance(
                                WalletCoinAdapter.AdapterType.SEND_ASSETS_LIST);
                bottomSheetDialogFragment.show(getSupportFragmentManager(),
                        EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
            });
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
            LinearLayout toSendSection = findViewById(R.id.to_send_section);
            toSendSection.setVisibility(View.GONE);
            currencySign.setVisibility(View.GONE);
            toValueText.setText("");
            toValueText.setHint("0");
            // Comment market, limit for now, but we may need it in the future
            // Button btMarket = findViewById(R.id.market_radio);
            // Button btLimit = findViewById(R.id.limit_radio);
            // TextView marketLimitPriceText = findViewById(R.id.market_limit_price_text);
            // EditText limitPriceValue = findViewById(R.id.limit_price_value);
            // TextView slippingExpiresValueText = findViewById(R.id.slipping_expires_value_text);
            // ImageView refreshPrice = findViewById(R.id.refresh_price);
            // btMarket.setOnClickListener(new View.OnClickListener() {
            //     @Override
            //     public void onClick(View v) {
            //         toEstimateText.setText(getText(R.string.to_estimate));
            //         marketLimitPriceText.setText(getText(R.string.market_price_in));
            //         marketPriceValueText.setVisibility(View.VISIBLE);
            //         limitPriceValue.setVisibility(View.GONE);
            //         marketPriceValueText.setVisibility(View.VISIBLE);
            //         slippingExpiresValueText.setText(getText(R.string.slipping_tolerance));
            //         slippingToleranceValueText.setText("2%");
            //         refreshPrice.setVisibility(View.VISIBLE);
            //     }
            // });
            // btLimit.setOnClickListener(new View.OnClickListener() {
            //     @Override
            //     public void onClick(View v) {
            //         toEstimateText.setText(getText(R.string.to_address));
            //         marketLimitPriceText.setText(getText(R.string.price_in));
            //         marketPriceValueText.setVisibility(View.GONE);
            //         limitPriceValue.setVisibility(View.VISIBLE);
            //         marketPriceValueText.setVisibility(View.GONE);
            //         slippingExpiresValueText.setText(getText(R.string.expires_in));
            //         slippingToleranceValueText.setText("1 days");
            //         refreshPrice.setVisibility(View.GONE);
            //     }
            // });
            assetFromDropDown.setOnClickListener(v -> {
                EditVisibleAssetsBottomSheetDialogFragment bottomSheetDialogFragment =
                        EditVisibleAssetsBottomSheetDialogFragment.newInstance(
                                WalletCoinAdapter.AdapterType.SEND_ASSETS_LIST);
                bottomSheetDialogFragment.show(getSupportFragmentManager(),
                        EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
            });
            TextView assetToDropDown = findViewById(R.id.to_asset_text);
            assetToDropDown.setOnClickListener(v -> {
                EditVisibleAssetsBottomSheetDialogFragment bottomSheetDialogFragment =
                        EditVisibleAssetsBottomSheetDialogFragment.newInstance(
                                WalletCoinAdapter.AdapterType.SWAP_ASSETS_LIST);
                bottomSheetDialogFragment.show(getSupportFragmentManager(),
                        EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
            });
            enableDisableSwapButton();
            Spinner spinner = findViewById(R.id.slippage_tolerance_spinner);
            spinner.setOnItemSelectedListener(this);
            // Creating adapter for a slippage tolerance spinner
            ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(this,
                    android.R.layout.simple_spinner_item, Utils.getSlippageToleranceList(this));
            dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            spinner.setAdapter(dataAdapter);
            ImageView refreshPrice = findViewById(R.id.refresh_price);
            refreshPrice.setOnClickListener(v -> { getSendSwapQuota(true, false); });
            EditText fromValueText = findViewById(R.id.from_value_text);
            fromValueText.addTextChangedListener(filterTextWatcherFrom);
            toValueText.addTextChangedListener(filterTextWatcherTo);
        }

        btnBuySendSwap.setOnClickListener(v -> {
            String from = mCustomAccountAdapter.getTitleAtPosition(
                    mAccountSpinner.getSelectedItemPosition());
            EditText fromValueText = findViewById(R.id.from_value_text);
            // TODO(sergz): Some kind of validation that we have enough balance
            String value = fromValueText.getText().toString();
            if (mActivityType == ActivityType.SEND) {
                EditText toSendValueText = findViewById(R.id.to_send_value_text);
                String to = toSendValueText.getText().toString();
                if (to.isEmpty()) {
                    // TODO(sergz): some address validation
                    return;
                }
                if (mCurrentErcToken == null || mCurrentErcToken.contractAddress.isEmpty()) {
                    TxData data =
                            Utils.getTxData("", "", "", to, Utils.toHexWei(value, 18), new byte[0]);
                    sendTransaction(data, from, "", "");
                } else {
                    addUnapprovedTransactionERC20(to,
                            Utils.toHexWei(value, mCurrentErcToken.decimals), from,
                            mCurrentErcToken.contractAddress);
                }
            } else if (mActivityType == ActivityType.BUY) {
                if (mCurrentChainId.equals(BraveWalletConstants.MAINNET_CHAIN_ID)) {
                    assert mErcTokenRegistry != null;
                    String asset = assetFromDropDown.getText().toString();
                    mErcTokenRegistry.getBuyUrl(from, asset, value, url -> {
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
                if (mCurrentErcToken != null) {
                    String btnText = btnBuySendSwap.getText().toString();
                    String toCompare = String.format(
                            getString(R.string.activate_erc20), mCurrentErcToken.symbol);
                    if (btnText.equals(toCompare)) {
                        activateErc20Allowance();

                        return;
                    }
                }
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

                EditText fromValueText = findViewById(R.id.from_value_text);
                fromValueText.setText(String.format(Locale.getDefault(), "%f", amountToGet));
                radioPerPercent.clearCheck();
                if (mActivityType == ActivityType.SWAP) {
                    getSendSwapQuota(true, false);
                }
            }
        });
    }

    @Override
    public void onUserInteraction() {
        if (mKeyringController == null) {
            return;
        }
        mKeyringController.notifyUserInteraction();
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
                EditText toSendText = findViewById(R.id.to_send_value_text);
                toSendText.setText(barcodeValue);
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

    private TextWatcher filterTextWatcherFrom = new TextWatcher() {
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            EditText fromValueText = findViewById(R.id.from_value_text);
            if (fromValueText.hasFocus()) {
                getSendSwapQuota(true, false);
            }
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
    };

    private TextWatcher filterTextWatcherTo = new TextWatcher() {
        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            EditText toValueText = findViewById(R.id.to_value_text);
            if (toValueText.hasFocus()) {
                getSendSwapQuota(false, false);
            }
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
    };

    private void activateErc20Allowance() {
        assert mAllowanceTarget != null && !mAllowanceTarget.isEmpty();
        assert mEthTxController != null;
        assert mCurrentErcToken != null;
        mEthTxController.makeErc20ApproveData(mAllowanceTarget,
                Utils.toHexWei(String.format(Locale.getDefault(), "%.4f", mConvertedFromBalance),
                        mCurrentErcToken.decimals),
                (success, data) -> {
                    if (!success) {
                        return;
                    }
                    TxData txData = Utils.getTxData(
                            "", "", "", mCurrentErcToken.contractAddress, "0x0", data);
                    String from = mCustomAccountAdapter.getTitleAtPosition(
                            mAccountSpinner.getSelectedItemPosition());
                    sendTransaction(txData, from, "", "");
                });
    }

    private void sendTransaction(
            TxData data, String from, String maxPriorityFeePerGas, String maxFeePerGas) {
        assert mEthJsonRpcController != null;
        mEthJsonRpcController.getAllNetworks(networks -> {
            boolean isEIP1559 = false;
            // We have hardcoded EIP-1559 gas fields.
            if (!maxPriorityFeePerGas.isEmpty() && !maxFeePerGas.isEmpty()) {
                isEIP1559 = true;
            } else if (!data.gasPrice.isEmpty()) {
                // We have hardcoded legacy tx gas fields.
                isEIP1559 = false;
            }
            for (EthereumChain network : networks) {
                if (!mCurrentChainId.equals(network.chainId)) {
                    continue;
                }
                isEIP1559 = network.isEip1559;
            }

            assert mEthTxController != null;
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
                mEthTxController.addUnapproved1559Transaction(txData1559, from,
                        (success, tx_meta_id, error_message)
                                -> {
                                        // Do nothing here as we will receive an
                                        // unapproved transaction in
                                        // EthTxControllerObserverImpl
                                });
            } else {
                mEthTxController.addUnapprovedTransaction(data, from,
                        (success, tx_meta_id, error_message)
                                -> {
                                        // Do nothing here as we will receive an
                                        // unapproved transaction in
                                        // EthTxControllerObserverImpl
                                });
            }
        });
    }

    private void addUnapprovedTransactionERC20(
            String to, String value, String from, String contractAddress) {
        assert mEthTxController != null;
        if (mEthTxController == null) {
            return;
        }
        mEthTxController.makeErc20TransferData(to, value, (success, data) -> {
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
            if (mCurrentErcToken != null) {
                Button btnBuySendSwap = findViewById(R.id.btn_buy_send_swap);
                String btnText = btnBuySendSwap.getText().toString();
                String toCompare =
                        String.format(getString(R.string.activate_erc20), mCurrentErcToken.symbol);
                if (btnText.equals(toCompare)) {
                    mActivateAllowanceTxId = txInfo.id;
                }
            }
        }
        ApproveTxBottomSheetDialogFragment approveTxBottomSheetDialogFragment =
                ApproveTxBottomSheetDialogFragment.newInstance(txInfo, accountName);
        approveTxBottomSheetDialogFragment.show(
                getSupportFragmentManager(), ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
    }

    public void showSwapButtonText() {
        Button btnBuySendSwap = findViewById(R.id.btn_buy_send_swap);
        btnBuySendSwap.setText(getString(R.string.swap));
    }

    public void updateBuySendAsset(String asset, ErcToken ercToken) {
        TextView assetFromDropDown = findViewById(R.id.from_asset_text);
        assetFromDropDown.setText(asset);
        mCurrentErcToken = ercToken;
        // Replace USDC and DAI contract addresses for Ropsten network
        mCurrentErcToken.contractAddress = Utils.getContractAddress(
                mCurrentChainId, mCurrentErcToken.symbol, mCurrentErcToken.contractAddress);
        String tokensPath = ERCTokenRegistryFactory.getInstance().getTokensIconsLocation();
        String iconPath =
                ercToken.logo.isEmpty() ? null : ("file://" + tokensPath + "/" + ercToken.logo);
        Utils.setBitmapResource(mExecutor, mHandler, this, iconPath, R.drawable.ic_eth_24, null,
                assetFromDropDown, true);
        updateBalance(
                mCustomAccountAdapter.getTitleAtPosition(mAccountSpinner.getSelectedItemPosition()),
                true);
        if (mActivityType == ActivityType.SWAP) {
            enableDisableSwapButton();
            getSendSwapQuota(true, false);
        }
    }

    public void updateSwapToAsset(String asset, ErcToken ercToken) {
        TextView assetToDropDown = findViewById(R.id.to_asset_text);
        assetToDropDown.setText(asset);
        mCurrentSwapToErcToken = ercToken;
        // Replace USDC and DAI contract addresses for Ropsten network
        mCurrentSwapToErcToken.contractAddress = Utils.getContractAddress(mCurrentChainId,
                mCurrentSwapToErcToken.symbol, mCurrentSwapToErcToken.contractAddress);
        String tokensPath = ERCTokenRegistryFactory.getInstance().getTokensIconsLocation();
        String iconPath =
                ercToken.logo.isEmpty() ? null : ("file://" + tokensPath + "/" + ercToken.logo);
        Utils.setBitmapResource(mExecutor, mHandler, this, iconPath, R.drawable.ic_eth_24, null,
                assetToDropDown, true);
        updateBalance(
                mCustomAccountAdapter.getTitleAtPosition(mAccountSpinner.getSelectedItemPosition()),
                false);
        enableDisableSwapButton();
        getSendSwapQuota(true, false);
    }

    private void enableDisableSwapButton() {
        boolean enable = true;
        if (mCurrentSwapToErcToken == null && mCurrentErcToken == null) {
            enable = false;
        } else if (mCurrentSwapToErcToken == null && mCurrentErcToken != null) {
            if (mCurrentErcToken.contractAddress.isEmpty()) {
                enable = false;
            }
        } else if (mCurrentErcToken == null && mCurrentSwapToErcToken != null) {
            if (mCurrentSwapToErcToken.contractAddress.isEmpty()) {
                enable = false;
            }
        } else if (mCurrentSwapToErcToken.contractAddress.equals(
                           mCurrentErcToken.contractAddress)) {
            enable = false;
        }

        Button btnBuySendSwap = findViewById(R.id.btn_buy_send_swap);
        btnBuySendSwap.setEnabled(enable);
    }

    public ErcTokenRegistry getErcTokenRegistry() {
        return mErcTokenRegistry;
    }

    @Override
    public void onConnectionError(MojoException e) {
        mKeyringController.close();
        mAssetRatioController.close();
        mErcTokenRegistry.close();
        mEthJsonRpcController.close();
        mEthTxController.close();
        mSwapController.close();

        mErcTokenRegistry = null;
        mEthJsonRpcController = null;
        mEthTxController = null;
        mKeyringController = null;
        mAssetRatioController = null;
        mSwapController = null;
        InitAssetRatioController();
        InitErcTokenRegistry();
        InitEthJsonRpcController();
        InitEthTxController();
        InitKeyringController();
        InitSwapController();
    }

    public AssetRatioController getAssetRatioController() {
        return mAssetRatioController;
    }

    public EthTxController getEthTxController() {
        return mEthTxController;
    }

    public EthJsonRpcController getEthJsonRpcController() {
        return mEthJsonRpcController;
    }

    private void InitAssetRatioController() {
        if (mAssetRatioController != null) {
            return;
        }

        mAssetRatioController =
                AssetRatioControllerFactory.getInstance().getAssetRatioController(this);
    }

    private void InitKeyringController() {
        if (mKeyringController != null) {
            return;
        }

        mKeyringController = KeyringControllerFactory.getInstance().getKeyringController(this);
    }

    private void InitErcTokenRegistry() {
        if (mErcTokenRegistry != null) {
            return;
        }

        mErcTokenRegistry = ERCTokenRegistryFactory.getInstance().getERCTokenRegistry(this);
    }

    private void InitEthJsonRpcController() {
        if (mEthJsonRpcController != null) {
            return;
        }

        mEthJsonRpcController =
                EthJsonRpcControllerFactory.getInstance().getEthJsonRpcController(this);
    }

    private void InitEthTxController() {
        if (mEthTxController != null) {
            return;
        }

        mEthTxController = EthTxControllerFactory.getInstance().getEthTxController(this);
        mEthTxControllerObserver = new EthTxControllerObserverImpl(this);
        mEthTxController.addObserver(mEthTxControllerObserver);
    }

    private void InitSwapController() {
        if (mSwapController != null) {
            return;
        }

        mSwapController = SwapControllerFactory.getInstance().getSwapController(this);
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
    }

    @Override
    public boolean shouldStartGpuProcess() {
        return true;
    }
}
