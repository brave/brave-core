/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.activities;

import static org.chromium.chrome.browser.crypto_wallet.util.Utils.warnWhenError;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.method.LinkMovementMethod;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.View.OnFocusChangeListener;
import android.widget.AdapterView;
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
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.Log;
import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.GasEstimation1559;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.ProviderError;
import org.chromium.brave_wallet.mojom.SolanaProviderError;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.brave_wallet.mojom.TxDataUnion;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveLocalState;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.app.domain.SendModel;
import org.chromium.chrome.browser.app.domain.WalletModel;
import org.chromium.chrome.browser.crypto_wallet.BlockchainRegistryFactory;
import org.chromium.chrome.browser.crypto_wallet.adapters.AccountSpinnerAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.NetworkSpinnerAdapter;
import org.chromium.chrome.browser.crypto_wallet.adapters.WalletCoinAdapter;
import org.chromium.chrome.browser.crypto_wallet.fragments.ApproveTxBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.fragments.EditVisibleAssetsBottomSheetDialogFragment;
import org.chromium.chrome.browser.crypto_wallet.model.WalletListItemModel;
import org.chromium.chrome.browser.crypto_wallet.observers.ApprovedTxObserver;
import org.chromium.chrome.browser.crypto_wallet.util.AddressUtils;
import org.chromium.chrome.browser.crypto_wallet.util.JavaUtils;
import org.chromium.chrome.browser.crypto_wallet.util.TokenUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.Validations;
import org.chromium.chrome.browser.crypto_wallet.util.WalletNativeUtils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletUtils;
import org.chromium.chrome.browser.decentralized_dns.EnsOffchainResolveMethod;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.qrreader.BarcodeTracker;
import org.chromium.chrome.browser.qrreader.BarcodeTrackerFactory;
import org.chromium.chrome.browser.qrreader.CameraSource;
import org.chromium.chrome.browser.qrreader.CameraSourcePreview;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.ui.text.NoUnderlineClickableSpan;

import java.io.IOException;
import java.text.ParseException;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BuySendSwapActivity extends BraveWalletBaseActivity
        implements AdapterView.OnItemSelectedListener, BarcodeTracker.BarcodeGraphicTrackerCallback,
                   ApprovedTxObserver {
    public static final String ACTIVITY_TYPE = "activityType";
    public static final String ASSET_CHAIN_ID = "ASSET_CHAIN";
    private static final String TAG = "BuySendSwap";
    private static final int RC_HANDLE_CAMERA_PERM = 113;
    // Intent request code to handle updating play services if needed.
    private static final int RC_HANDLE_GMS = 9001;

    private CameraSource mCameraSource;
    private CameraSourcePreview mCameraSourcePreview;
    private boolean mInitialLayoutInflationComplete;

    private List<AccountInfo> mAllAccountInfos;
    private List<AccountInfo> mAccountInfos;
    private SendModel mSendModel;
    private List<NetworkInfo> mNetworks;
    private String mIntentChainId;
    private ActivityType mActivityType;
    private AccountSpinnerAdapter mCustomAccountAdapter;
    private double mConvertedFromBalance;
    private double mConvertedToBalance;
    private BlockchainToken mCurrentBlockchainToken;
    private ExecutorService mExecutor;
    private Handler mHandler;
    private String mAllowanceTarget;
    private String mCurrentChainId;
    private NetworkInfo mSelectedNetwork;
    private AccountInfo mSelectedAccount;
    private WalletModel mWalletModel;

    private Spinner mAccountSpinner;
    private Button mBtnBuySend;
    private EditText mFromValueText;
    private EditText mSendToAddrText;
    private TextView mResolvedAddrText;
    private LinearLayout mEnsOffchainLookupSection;
    private LinearLayout mSearchingForDomainSection;
    private TextView mFromBalanceText;
    private TextView mToBalanceText;
    private TextView mFromAssetText;
    private TextView mSendToValidation;
    private TextView mFromSendValueValidation;
    private LinearLayout mFromValueBlock;
    private Spinner mNetworkSpinner;

    private NetworkSpinnerAdapter mNetworkAdapter;

    @Override
    public void onDestroy() {
        if (mCameraSourcePreview != null) {
            mCameraSourcePreview.release();
        }
        mWalletModel.getCryptoModel().clearBSS();
        super.onDestroy();
    }

    @Override
    @SuppressLint("SetTextI18n")
    protected void triggerLayoutInflation() {
        setContentView(R.layout.activity_buy_send_swap);

        mAccountSpinner = findViewById(R.id.accounts_spinner);
        Intent intent = getIntent();
        mActivityType = ActivityType.valueOf(
                intent.getIntExtra(ACTIVITY_TYPE, ActivityType.BUY.getValue()));

        mExecutor = Executors.newSingleThreadExecutor();
        mHandler = new Handler(Looper.getMainLooper());

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mFromValueText = findViewById(R.id.from_value_text);
        mFromValueText.setText("");
        mFromValueText.setHint("0");

        mFromBalanceText = findViewById(R.id.from_balance_text);
        mFromAssetText = findViewById(R.id.from_asset_text);

        mBtnBuySend = findViewById(R.id.btn_buy_send_swap);

        mSendToValidation = findViewById(R.id.to_send_error_text);
        mFromSendValueValidation = findViewById(R.id.from_send_value_error_text);

        mFromValueBlock = findViewById(R.id.from_value_block);

        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            mWalletModel = activity.getWalletModel();
            mSendModel = mWalletModel.getCryptoModel().createSendModel();
            mIntentChainId = intent.getStringExtra(ASSET_CHAIN_ID);
            updateNetworkPerAssetChain(mIntentChainId);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "triggerLayoutInflation " + e);
        }

        mNetworkSpinner = findViewById(R.id.network_spinner);
        mNetworkAdapter = new NetworkSpinnerAdapter(this, Collections.emptyList());
        mNetworkSpinner.setAdapter(mNetworkAdapter);
        mNetworkSpinner.setOnItemSelectedListener(this);
        mCustomAccountAdapter =
                new AccountSpinnerAdapter(getApplicationContext(), Collections.emptyList());
        mAccountSpinner.setAdapter(mCustomAccountAdapter);

        onInitialLayoutInflationComplete();
        mInitialLayoutInflationComplete = true;

        adjustControls();
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        if (parent.getId() == R.id.network_spinner) {
            if (isSameSelectedNetwork(mSelectedNetwork)) return;

            NetworkInfo networkInfo = mNetworkAdapter.getNetwork(position);
            if (networkInfo != null) {
                // Shall be fine regardless of activity type
                mFromValueText.setText("");
                mFromValueText.setHint("0");

                mWalletModel.getCryptoModel().getNetworkModel().setNetworkWithAccountCheck(
                        networkInfo, success -> {
                            if (!success) {
                                return;
                            }
                            mSelectedNetwork = networkInfo;
                        });
            }

        } else if (parent.getId() == R.id.accounts_spinner) {
            AccountInfo accountInfo = mCustomAccountAdapter.getSelectedAccountAt(position);
            mWalletModel.getKeyringModel().setSelectedAccount(accountInfo);
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> arg0) {}

    private void resetFromAssets() {
        if (mBlockchainRegistry == null || mCustomAccountAdapter == null
                || !mInitialLayoutInflationComplete)
            return;
        final BlockchainToken nativeAsset = Utils.makeNetworkAsset(mSelectedNetwork);
        // From
        final String fromAssetSymbol = nativeAsset.symbol;
        TokenUtils.getAllTokensFiltered(mBraveWalletService, mBlockchainRegistry, mSelectedNetwork,
                mSelectedNetwork.coin,
                nativeAsset.coin == CoinType.SOL ? TokenUtils.TokenType.SOL
                                                 : TokenUtils.TokenType.ERC20,
                allTokens -> {
                    if (fromAssetSymbol.equals(nativeAsset.symbol)) {
                        updateBuySendAsset(nativeAsset.symbol, nativeAsset);
                        updateBalance(getCurrentSelectedAccountAddr());
                    } else {
                        mBlockchainRegistry.getTokenBySymbol(mSelectedNetwork.chainId,
                                mSelectedNetwork.coin, fromAssetSymbol, token -> {
                                    if (token != null) {
                                        updateBuySendAsset(token.symbol, token);
                                        updateBalance(getCurrentSelectedAccountAddr());
                                        return;
                                    }
                                    // We most likely have a custom token
                                    for (BlockchainToken filteredToken : allTokens) {
                                        if (fromAssetSymbol.equals(filteredToken.symbol)) {
                                            updateBuySendAsset(filteredToken.symbol, filteredToken);
                                            updateBalance(getCurrentSelectedAccountAddr());
                                            break;
                                        }
                                    }
                                });
                    }
                });
    }

    private void clearFromAssetState() {
        mCurrentBlockchainToken = null;
        clearBalanceAssetUi(mFromBalanceText, mFromAssetText);
    }

    private void clearBalanceAssetUi(TextView balance, TextView asset) {
        balance.setText("0");
        asset.setText(null);
        asset.setCompoundDrawablesWithIntrinsicBounds(0, 0, 0, 0);
    }

    private void updateBalance(String address) {
        assert mJsonRpcService != null;
        if (mJsonRpcService == null) {
            return;
        }
        final BlockchainToken blockchainToken = mCurrentBlockchainToken;
        if (blockchainToken == null) return;

        if (blockchainToken.contractAddress.isEmpty()) {
            if (mSelectedAccount.accountId.coin == CoinType.ETH
                    || mSelectedAccount.accountId.coin == CoinType.FIL) {
                mJsonRpcService.getBalance(address, mSelectedAccount.accountId.coin,
                        mSelectedNetwork.chainId, (balance, error, errorMessage) -> {
                            warnWhenError(TAG, "getBalance", error, errorMessage);
                            if (error != ProviderError.SUCCESS
                                    || mSelectedNetwork.coin != mSelectedAccount.accountId.coin
                                    || !mSelectedAccount.address.equals(address)) {
                                return;
                            }
                            populateBalance(balance, 0);
                        });
            } else if (mSelectedNetwork.coin == CoinType.SOL
                    && Utils.isNativeToken(mSelectedNetwork, blockchainToken)) {
                mJsonRpcService.getSolanaBalance(
                        address, mSelectedNetwork.chainId, (balance, error, errorMessage) -> {
                            warnWhenError(TAG, "getSolanaBalance", error, errorMessage);
                            if (error != SolanaProviderError.SUCCESS
                                    || mSelectedNetwork.coin != CoinType.SOL
                                    || !mSelectedAccount.address.equals(address)) {
                                return;
                            }
                            populateBalance(String.valueOf(balance), 0);
                        });
            }
        } else if (blockchainToken.isErc721) {
            mJsonRpcService.getErc721TokenBalance(blockchainToken.contractAddress,
                    blockchainToken.tokenId, address, mSelectedNetwork.chainId,
                    (balance, error, errorMessage) -> {
                        warnWhenError(TAG, "getERC721TokenBalance", error, errorMessage);
                        if (error != ProviderError.SUCCESS || !blockchainToken.isErc721
                                || !mSelectedAccount.address.equals(address)) {
                            return;
                        }
                        populateBalance(balance, 0);
                    });
        } else if (mSelectedNetwork.coin == CoinType.SOL
                && !TextUtils.isEmpty(blockchainToken.contractAddress)) {
            // TODO(pav) : fix why solana tx has erc20 as true
            mJsonRpcService.getSplTokenAccountBalance(address, blockchainToken.contractAddress,
                    mSelectedNetwork.chainId,
                    (amount, decimals, uiAmountString, solanaProvideError, error_message) -> {
                        if (solanaProvideError != SolanaProviderError.SUCCESS
                                || mSelectedNetwork.coin != CoinType.SOL
                                || !mSelectedAccount.address.equals(address)) {
                            return;
                        }
                        populateBalance(amount, decimals);
                    });
        } else {
            mJsonRpcService.getErc20TokenBalance(blockchainToken.contractAddress, address,
                    mSelectedNetwork.chainId, (balance, error, errorMessage) -> {
                        warnWhenError(TAG, "getErc20TokenBalance", error, errorMessage);
                        if (error != ProviderError.SUCCESS || !blockchainToken.isErc20
                                || !mSelectedAccount.address.equals(address)) {
                            return;
                        }
                        populateBalance(balance, 0);
                    });
        }
    }

    public String getCurrentSelectedAccountAddr() {
        return mCustomAccountAdapter.getAccountAddressAtPosition(
                mAccountSpinner.getSelectedItemPosition());
    }

    private void populateBalance(String balance, int responseDecimals) {
        BlockchainToken token = mCurrentBlockchainToken;
        TextView textView = mFromBalanceText;

        double fromToBalance = 0;

        // some old calls are returning ETH result when SOL is selected so do nothing
        try {
            int decimals = responseDecimals;
            if (decimals == 0) {
                decimals = token.decimals;
            }
            int tokenCoin = token.coin;
            fromToBalance = Utils.getBalanceForCoinType(tokenCoin, decimals, balance);
        } catch (NumberFormatException | NullPointerException e) {
            Log.e(TAG, "Error while parsing token balance.", e);
            clearFromAssetState();
            return;
        }

        String text = getText(R.string.crypto_wallet_balance) + " "
                + String.format(Locale.getDefault(), "%.4f", fromToBalance);
        textView.setText(text);
        mConvertedFromBalance = fromToBalance;
        if (mActivityType == ActivityType.SEND) mBtnBuySend.setEnabled(fromToBalance != 0);
    }

    private void setSelectedNetwork(NetworkInfo networkInfo, List<NetworkInfo> networkInfos) {
        if (isSameSelectedNetwork(networkInfo)) return;
        mNetworkSpinner.setOnItemSelectedListener(null);
        mNetworkSpinner.setSelection(getIndexOf(networkInfo, networkInfos), false);
        mNetworkSpinner.setOnItemSelectedListener(this);
    }

    private boolean isSameSelectedNetwork(NetworkInfo networkInfo) {
        NetworkInfo currSelected = (NetworkInfo) mNetworkSpinner.getSelectedItem();
        if (currSelected != null && networkInfo.chainId.equals(currSelected.chainId)
                && networkInfo.coin == currSelected.coin)
            return true;
        return false;
    }

    private int getIndexOf(NetworkInfo selectedNetwork, List<NetworkInfo> networksInfos) {
        for (int i = 0; i < networksInfos.size(); i++) {
            NetworkInfo networkInfo = networksInfos.get(i);
            if (networkInfo.chainId.equals(selectedNetwork.chainId)
                    && networkInfo.coin == selectedNetwork.coin) {
                return i;
            }
        }
        return 0;
    }

    private void setSendToFromValueValidationResult(
            String validationResult, boolean disableButtonOnError, boolean sendTo) {
        boolean validationSucceeded = (validationResult == null || validationResult.isEmpty());
        final TextView thisValueValidation = sendTo ? mSendToValidation : mFromSendValueValidation;
        final TextView otherValueValidation = sendTo ? mFromSendValueValidation : mSendToValidation;

        boolean otherValidationError = otherValueValidation.getVisibility() == View.VISIBLE;
        boolean buttonShouldBeEnabled = validationSucceeded || (sendTo && !disableButtonOnError);
        mBtnBuySend.setEnabled(!otherValidationError && buttonShouldBeEnabled);

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
        TextView currencySign = findViewById(R.id.currency_sign);
        TextView toEstimateText = findViewById(R.id.to_estimate_text);
        RadioGroup radioPerPercent = findViewById(R.id.per_percent_radiogroup);
        radioPerPercent.clearCheck();

        // Common
        WalletCoinAdapter.AdapterType fromAdapterType = mActivityType == ActivityType.BUY
                ? WalletCoinAdapter.AdapterType.BUY_ASSETS_LIST
                : WalletCoinAdapter.AdapterType.SEND_ASSETS_LIST;
        mFromAssetText.setOnClickListener(v -> {
            EditVisibleAssetsBottomSheetDialogFragment bottomSheetDialogFragment =
                    EditVisibleAssetsBottomSheetDialogFragment.newInstance(fromAdapterType, false);

            bottomSheetDialogFragment.setOnAssetClickListener(
                    new EditVisibleAssetsBottomSheetDialogFragment
                            .OnEditVisibleItemClickListener() {
                                @Override
                                public void onAssetClick(WalletListItemModel asset) {
                                    updateBuySendAsset(
                                            asset.getSubTitle(), asset.getBlockchainToken());
                                    updateBalance(getCurrentSelectedAccountAddr());
                                }
                            });
            bottomSheetDialogFragment.setSelectedNetwork(mSelectedNetwork);
            bottomSheetDialogFragment.show(getSupportFragmentManager(),
                    EditVisibleAssetsBottomSheetDialogFragment.TAG_FRAGMENT);
        });

        mResolvedAddrText = findViewById(R.id.resolved_addr_text);
        mEnsOffchainLookupSection = findViewById(R.id.ens_offchain_lookup_section);
        mSearchingForDomainSection = findViewById(R.id.searching_for_domain_section);

        // Individual
        if (mActivityType == ActivityType.BUY) {
            TextView fromBuyText = findViewById(R.id.from_buy_text);
            fromBuyText.setText(getText(R.string.buy_wallet));
            LinearLayout toSection = findViewById(R.id.to_section);
            toSection.setVisibility(View.GONE);
            mBtnBuySend.setText(getText(R.string.buy_wallet));
            radioPerPercent.setVisibility(View.GONE);
        } else if (mActivityType == ActivityType.SEND) {
            currencySign.setVisibility(View.GONE);
            toEstimateText.setText(getText(R.string.to_address));
            mSendToAddrText = findViewById(R.id.send_to_addr_text);
            mSendToAddrText.setText("");
            mSendToAddrText.setHint(getText(R.string.to_address_edit));
            mSendToAddrText.addTextChangedListener(new FilterTextWatcherSendToAddr());
            mSendToAddrText.setOnFocusChangeListener(new OnFocusChangeListenerToSend());
            mFromValueText.setText("");
            mFromValueText.addTextChangedListener(new FilterTextWatcherFromSendValue());

            mBtnBuySend.setText(getText(R.string.send));

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
        }

        mBtnBuySend.setOnClickListener(v -> {
            String from = mCustomAccountAdapter.getAccountAddressAtPosition(
                    mAccountSpinner.getSelectedItemPosition());
            // TODO(sergz): Some kind of validation that we have enough balance
            String amount = mFromValueText.getText().toString();
            if (mActivityType == ActivityType.SEND) {
                String to = mSendToAddrText.getText().toString();
                if (!mResolvedAddrText.getText().toString().isEmpty()) {
                    to = mResolvedAddrText.getText().toString();
                }

                if (to.isEmpty()) {
                    Log.e(TAG, "Destination address cannot be empty.");
                    return;
                }
                if (mSelectedAccount.accountId.coin == CoinType.ETH) {
                    if (mCurrentBlockchainToken == null
                            || mCurrentBlockchainToken.contractAddress.isEmpty()) {
                        TxData data = Utils.getTxData("", "", "", to,
                                Utils.toHexWei(amount, Utils.ETH_DEFAULT_DECIMALS), new byte[0]);
                        sendTransaction(data, from, "", "");
                    } else if (mCurrentBlockchainToken.isErc20) {
                        addUnapprovedTransactionERC20(to,
                                Utils.toHexWei(amount, mCurrentBlockchainToken.decimals), from,
                                mCurrentBlockchainToken.contractAddress);
                    } else if (mCurrentBlockchainToken.isErc721) {
                        addUnapprovedTransactionERC721(from, to, mCurrentBlockchainToken.tokenId,
                                mCurrentBlockchainToken.contractAddress);
                    }
                } else if (mSelectedAccount.accountId.coin == CoinType.SOL) {
                    if (mCurrentBlockchainToken.isNft) {
                        amount = "1";
                    }
                    mSendModel.sendSolanaToken(mSelectedNetwork.chainId, mCurrentBlockchainToken,
                            mSelectedAccount.address, to,
                            Utils.toDecimalLamport(amount, mCurrentBlockchainToken.decimals),
                            (success, txMetaId, errorMessage) -> {
                                // Do nothing here when success as we will receive an
                                // unapproved transaction in TxServiceObserver.
                                // When we have error, let the user know,
                                // error_message is localized, do not disable send button
                                setSendToFromValueValidationResult(errorMessage, false, true);
                            });
                } else if (mSelectedAccount.accountId.coin == CoinType.FIL) {
                    mSendModel.sendFilecoinToken(mSelectedNetwork.chainId, mCurrentBlockchainToken,
                            mSelectedAccount.address, to, amount,
                            (success, txMetaId, errorMessage) -> {
                                // Do nothing here when success as we will receive an
                                // unapproved transaction in TxServiceObserver.
                                // When we have error, let the user know,
                                // error_message is localized, do not disable send button
                                setSendToFromValueValidationResult(errorMessage, false, true);
                            });
                }
            } else if (mActivityType == ActivityType.BUY) {
                Intent selectPurchaseMethodIntent = SelectPurchaseMethodActivity.getIntent(this,
                        mCurrentBlockchainToken.chainId, from, mCurrentBlockchainToken.symbol,
                        mCurrentBlockchainToken.contractAddress, amount);
                startActivity(selectPurchaseMethodIntent);
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

                mFromValueText.setText(String.format(Locale.getDefault(), "%.8f", amountToGet));
                radioPerPercent.clearCheck();
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
        String barcodeValue = barcode.displayValue;
        if (mSelectedNetwork.coin == CoinType.ETH) {
            barcodeValue = AddressUtils.sanitizeEthAddress(barcodeValue);
        }
        final String finalBarcodeValue = barcodeValue;
        runOnUiThread(() -> {
            if (null != mCameraSourcePreview) {
                mCameraSourcePreview.stop();
            }
            RelativeLayout relativeLayout = findViewById(R.id.camera_layout);
            relativeLayout.setVisibility(View.GONE);
            mSendToAddrText.setText(finalBarcodeValue);
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

    private void onResolveWalletAddressDone(
            String domain, String result, Boolean requireEnsOffchainConsent) {
        mSearchingForDomainSection.setVisibility(View.GONE);

        if (!domain.equals(mSendToAddrText.getText().toString())) {
            return;
        }

        if (requireEnsOffchainConsent) {
            TextView ensOffchainLookupDescriptionText =
                    findViewById(R.id.ens_offchain_lookup_description_text);
            ensOffchainLookupDescriptionText.setVisibility(View.VISIBLE);
            ensOffchainLookupDescriptionText.setMovementMethod(LinkMovementMethod.getInstance());
            String learnMoreText = getString(R.string.brave_wallet_ens_off_chain_lookup_learn_more);
            String descriptionText = getString(
                    R.string.brave_wallet_ens_off_chain_lookup_description, learnMoreText);

            NoUnderlineClickableSpan span =
                    new NoUnderlineClickableSpan(this, R.color.brave_action_color, (textView) -> {
                        TabUtils.openUrlInNewTab(false, Utils.ENS_OFFCHAIN_LEARN_MORE_URL);
                        TabUtils.bringChromeTabbedActivityToTheTop(BuySendSwapActivity.this);
                    });

            ensOffchainLookupDescriptionText.setText(Utils.createSpannableString(
                    descriptionText, learnMoreText, span, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE));

            Button mBtnEnableEnsOffchainLookup = findViewById(R.id.btn_enable_ens_offchain_lookup);
            mBtnEnableEnsOffchainLookup.setOnClickListener(v -> {
                BraveLocalState.get().setInteger(
                        BravePref.ENS_OFFCHAIN_RESOLVE_METHOD, EnsOffchainResolveMethod.ENABLED);
                mEnsOffchainLookupSection.setVisibility(View.GONE);
                maybeResolveWalletAddress();
            });

            mEnsOffchainLookupSection.setVisibility(View.VISIBLE);
            setSendToFromValueValidationResult("", false, true);
            mBtnBuySend.setEnabled(false);
            return;
        }

        mEnsOffchainLookupSection.setVisibility(View.GONE);

        if (result == null || result.isEmpty()) {
            mResolvedAddrText.setVisibility(View.GONE);
            mResolvedAddrText.setText("");
            String notRegisteredErrorText = String.format(
                    getString(R.string.wallet_domain_not_registered_error_text), domain);

            setSendToFromValueValidationResult(notRegisteredErrorText, true, true);
        } else {
            mResolvedAddrText.setVisibility(View.VISIBLE);
            mResolvedAddrText.setText(result);
            setSendToFromValueValidationResult("", false, true);
        }
    }

    private boolean maybeResolveWalletAddress() {
        String domain = mSendToAddrText.getText().toString();

        if (WalletNativeUtils.isUnstoppableDomainsTld(domain)) {
            mSearchingForDomainSection.setVisibility(View.VISIBLE);
            mSendToValidation.setText("");
            mSendToValidation.setVisibility(View.GONE);
            mJsonRpcService.unstoppableDomainsGetWalletAddr(
                    domain, mCurrentBlockchainToken, (response, errorResponse, errorString) -> {
                        onResolveWalletAddressDone(domain, response, false);
                    });
            return true;
        }

        if (mCurrentBlockchainToken.coin == CoinType.ETH && WalletNativeUtils.isEnsTld(domain)) {
            mSearchingForDomainSection.setVisibility(View.VISIBLE);
            mSendToValidation.setText("");
            mSendToValidation.setVisibility(View.GONE);
            mJsonRpcService.ensGetEthAddr(
                    domain, (response, requireOffchainConsent, errorResponse, errorString) -> {
                        onResolveWalletAddressDone(domain, response, requireOffchainConsent);
                    });
            return true;
        }

        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_WALLET_SNS)) {
            mSearchingForDomainSection.setVisibility(View.VISIBLE);
            mSendToValidation.setText("");
            mSendToValidation.setVisibility(View.GONE);
            if (mCurrentBlockchainToken.coin == CoinType.SOL
                    && WalletNativeUtils.isSnsTld(domain)) {
                mJsonRpcService.snsGetSolAddr(domain, (response, errorResponse, errorString) -> {
                    onResolveWalletAddressDone(domain, response, false);
                });
                return true;
            }
        }

        mSearchingForDomainSection.setVisibility(View.GONE);
        mEnsOffchainLookupSection.setVisibility(View.GONE);
        mResolvedAddrText.setVisibility(View.GONE);
        mResolvedAddrText.setText("");
        return false;
    }

    public enum ActivityType {
        BUY(0),
        SEND(1),
        SWAP_V2(2);

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

    private class FilterTextWatcherSendToAddr implements TextWatcher {
        Validations.SendToAccountAddress mValidator;

        public FilterTextWatcherSendToAddr() {
            mValidator = new Validations.SendToAccountAddress();
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            if (maybeResolveWalletAddress()) return;

            String fromAccountAddress = mCustomAccountAdapter.getAccountAddressAtPosition(
                    mAccountSpinner.getSelectedItemPosition());

            mValidator.validate(mSelectedNetwork, getKeyringService(), getBlockchainRegistry(),
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
                if (maybeResolveWalletAddress()) return;

                String fromAccountAddress = mCustomAccountAdapter.getAccountAddressAtPosition(
                        mAccountSpinner.getSelectedItemPosition());
                String receiverAccountAddress = ((EditText) v).getText().toString();

                mValidator.validate(mSelectedNetwork, getKeyringService(), getBlockchainRegistry(),
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
                fromSendValue = Utils.parseDouble(s.toString());
                String validationResult = (fromSendValue > mConvertedFromBalance)
                        ? getString(R.string.crypto_wallet_error_insufficient_balance)
                        : "";
                setSendToFromValueValidationResult(validationResult, false, false);
            } catch (ParseException e) {
                Log.w(TAG, e.getMessage());
                setSendToFromValueValidationResult(
                        getString(R.string.crypto_wallet_error_wrong_number_format), true, false);
            }
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
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
            } else {
                for (NetworkInfo network : networks) {
                    if (!mSelectedNetwork.chainId.equals(network.chainId)) {
                        continue;
                    }
                    isEIP1559 = network.isEip1559;
                    break;
                }
            }

            assert mTxService != null;
            TxDataUnion txDataUnion = new TxDataUnion();
            if (isEIP1559) {
                TxData1559 txData1559 = new TxData1559();
                txData1559.baseData = data;
                txData1559.chainId = mSelectedNetwork.chainId;
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
                txDataUnion.setEthTxData1559(txData1559);
            } else {
                txDataUnion.setEthTxData(data);
            }
            mTxService.addUnapprovedTransaction(
                    txDataUnion, from, null, null, (success, tx_meta_id, error_message) -> {
                        // Do nothing here when success as we will receive an
                        // unapproved transaction in TxServiceObserver.
                        // When we have error, let the user know,
                        // error_message is localized, do not disable send button
                        setSendToFromValueValidationResult(error_message, false, true);
                    });
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

    private void addUnapprovedTransactionERC721(
            String from, String to, String tokenId, String contractAddress) {
        assert mEthTxManagerProxy != null;
        if (mEthTxManagerProxy == null) {
            return;
        }
        mEthTxManagerProxy.makeErc721TransferFromData(
                from, to, tokenId, contractAddress, (success, data) -> {
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
        ApproveTxBottomSheetDialogFragment approveTxBottomSheetDialogFragment =
                ApproveTxBottomSheetDialogFragment.newInstance(txInfo, accountName);
        approveTxBottomSheetDialogFragment.setApprovedTxObserver(this);
        approveTxBottomSheetDialogFragment.show(
                getSupportFragmentManager(), ApproveTxBottomSheetDialogFragment.TAG_FRAGMENT);
    }

    @Override
    public void onTxApprovedRejected(boolean approved, String accountName, String txId) {
        if (approved) {
            finish();
        }
    }

    @Override
    public void onTxPending(String accountName, String txId) {}

    public void updateBuySendAsset(String asset, BlockchainToken blockchainToken) {
        TextView assetText = mFromAssetText;
        assetText.setText(asset);
        mCurrentBlockchainToken = blockchainToken;

        BlockchainToken token = mCurrentBlockchainToken;
        // Replace USDC and DAI contract addresses for Goerli network
        token.contractAddress = Utils.getContractAddress(
                mSelectedNetwork.chainId, token.symbol, token.contractAddress);
        String tokensPath = BlockchainRegistryFactory.getInstance().getTokensIconsLocation();
        if (token.symbol.equals("ETH")) {
            token.logo = "eth.png";
        }
        String iconPath =
                blockchainToken.logo.isEmpty() ? null : ("file://" + tokensPath + "/" + token.logo);
        if (!token.logo.isEmpty()) {
            Utils.setBitmapResource(mExecutor, mHandler, this, iconPath,
                    Utils.getCoinIcon(token.coin), null, assetText, true);
        } else {
            Utils.setBlockiesBitmapCustomAsset(mExecutor, mHandler, null, token.contractAddress,
                    token.symbol, getResources().getDisplayMetrics().density, assetText, this, true,
                    (float) 0.5, true);
        }
        if (token.isNft) {
            mFromValueBlock.setVisibility(View.GONE);
        }

        if (mActivityType == ActivityType.SEND) {
            maybeResolveWalletAddress();
        }
    }

    private void initAccountsUI() {
        if (mSelectedAccount == null || mSelectedNetwork == null) {
            return;
        }
        if (mActivityType == ActivityType.BUY) {
            mAccountInfos = JavaUtils.filter(
                    mAllAccountInfos, item -> item.accountId.coin == mSelectedNetwork.coin);
        }

        mCustomAccountAdapter.setAccounts(mAccountInfos);
        if (mAccountInfos.size() > 0) {
            mAccountSpinner.setOnItemSelectedListener(null);
            mAccountSpinner.setSelection(
                    WalletUtils.getSelectedAccountIndex(mSelectedAccount, mAccountInfos), false);
        }
        mAccountSpinner.setOnItemSelectedListener(this);
        // Before updating, make sure we are on a network with the same coin type
        if (mSelectedNetwork != null && mSelectedAccount.accountId.coin == mSelectedNetwork.coin) {
            resetFromAssets();
        }
    }

    @Override
    public void finishNativeInitialization() {
        super.finishNativeInitialization();
        assert mWalletModel != null;
        mWalletModel.getCryptoModel().getNetworkModel().mChainNetworkAllNetwork.observe(
                this, chainNetworkAllNetwork -> {
                    if (JavaUtils.anyNull(
                                chainNetworkAllNetwork.first, chainNetworkAllNetwork.second))
                        return;
                    mCurrentChainId = chainNetworkAllNetwork.first;
                    mSelectedNetwork = chainNetworkAllNetwork.second;
                    mNetworks = mActivityType != ActivityType.SEND
                            ? mWalletModel.getCryptoModel().getNetworkModel().stripNoBuyNetworks(
                                    chainNetworkAllNetwork.third, mActivityType)
                            : chainNetworkAllNetwork.third;
                    mNetworkAdapter.setNetworks(mNetworks);
                    setSelectedNetwork(mSelectedNetwork, mNetworks);

                    mFromAssetText.setText(mSelectedNetwork.symbol);
                    Utils.setBlockiesBitmapCustomAsset(mExecutor, mHandler, null, "",
                            mSelectedNetwork.symbol, getResources().getDisplayMetrics().density,
                            mFromAssetText, this, true, (float) 0.5, true);

                    // Before updating, make sure we are on a network with the same coin type
                    if (mSelectedAccount != null
                            && mSelectedAccount.accountId.coin == mSelectedNetwork.coin) {
                        resetFromAssets();
                    }
                    if (mActivityType == ActivityType.BUY) {
                        mBtnBuySend.setText(getString(R.string.wallet_buy_mainnet_button_text));
                    }
                    initAccountsUI();
                });
        mWalletModel.getKeyringModel().mAccountAllAccountsPair.observe(
                this, accountInfoListPair -> {
                    mAllAccountInfos = accountInfoListPair.second;
                    mSelectedAccount = accountInfoListPair.first;
                    mAccountInfos = accountInfoListPair.second;

                    initAccountsUI();
                });

        mWalletModel.getCryptoModel().getNetworkModel().mNeedToCreateAccountForNetwork.observe(
                this, networkInfo -> {
                    if (networkInfo == null) {
                        return;
                    }

                    MaterialAlertDialogBuilder builder =
                            new MaterialAlertDialogBuilder(
                                    this, R.style.BraveWalletAlertDialogTheme)
                                    .setMessage(getString(
                                            R.string.brave_wallet_create_account_description,
                                            networkInfo.symbolName))
                                    .setPositiveButton(R.string.brave_action_yes,
                                            (dialog, which) -> {
                                                mWalletModel.getCryptoModel()
                                                        .getNetworkModel()
                                                        .setNetwork(networkInfo, success -> {
                                                            if (success) {
                                                                mWalletModel.getKeyringModel()
                                                                        .addAccount(
                                                                                networkInfo.coin,
                                                                                networkInfo.chainId,
                                                                                null,
                                                                                isAccountAdded
                                                                                -> {});
                                                            }
                                                            mWalletModel.getCryptoModel()
                                                                    .getNetworkModel()
                                                                    .clearCreateAccountState();
                                                        });
                                            })
                                    .setNegativeButton(
                                            R.string.brave_action_no, (dialog, which) -> {
                                                // update mNetworkSpinner to current network
                                                setSelectedNetwork(mSelectedNetwork, mNetworks);

                                                mWalletModel.getCryptoModel()
                                                        .getNetworkModel()
                                                        .clearCreateAccountState();
                                                dialog.dismiss();
                                            });
                    builder.show();
                });
    }

    @Override
    public void onNewUnapprovedTx(TransactionInfo txInfo) {
        showApproveTransactionDialog(txInfo);
    }

    @Override
    public void onTransactionStatusChanged(TransactionInfo txInfo) {}

    private void updateNetworkPerAssetChain(String assetChainId) {
        if (TextUtils.isEmpty(assetChainId)) return;
        mWalletModel.getCryptoModel().getNetworkModel().setNetworkWithAccountCheck(
                assetChainId, hasSetNetwork -> {});
    }
}
