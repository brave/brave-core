/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.app.Activity;
import android.app.Dialog;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import org.chromium.brave_wallet.mojom.AssetPriceTimeframe;
import org.chromium.brave_wallet.mojom.AssetRatioService;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.AdvanceTxSettingActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;

import java.util.Locale;

public class TxFragment extends Fragment {
    private TransactionInfo mTxInfo;
    private String mAsset;
    private int mDecimals;
    private String mChainSymbol;
    private int mChainDecimals;
    private double mTotalPrice;
    private boolean mIsEIP1559;
    private int mCheckedPriorityId;
    private int mPreviousCheckedPriorityId;
    private double mEthRate;
    public static final int START_ADVANCE_SETTING_ACTIVITY_CODE = 0;

    public static TxFragment newInstance(TransactionInfo txInfo, String asset, int decimals,
            String chainSymbol, int chainDecimals, double totalPrice) {
        return new TxFragment(txInfo, asset, decimals, chainSymbol, chainDecimals, totalPrice);
    }

    private AssetRatioService getAssetRatioService() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getAssetRatioService();
        }
        return null;
    }

    private EthTxManagerProxy getEthTxManagerProxy() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getEthTxManagerProxy();
        }
        return null;
    }

    private TxFragment(TransactionInfo txInfo, String asset, int decimals, String chainSymbol,
            int chainDecimals, double totalPrice) {
        mTxInfo = txInfo;
        mAsset = asset;
        mDecimals = decimals;
        mChainSymbol = chainSymbol;
        mChainDecimals = chainDecimals;
        mTotalPrice = totalPrice;
        mEthRate = 0;
        mCheckedPriorityId = -1;
        mPreviousCheckedPriorityId = -1;
        mIsEIP1559 = false;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.fragment_transaction, container, false);
        mIsEIP1559 = !mTxInfo.txDataUnion.getEthTxData1559().maxPriorityFeePerGas.isEmpty()
                && !mTxInfo.txDataUnion.getEthTxData1559().maxFeePerGas.isEmpty();

        setupView(view);

        View advanceSettingContainer = view.findViewById(R.id.fragment_tx_tv_advance_setting);
        advanceSettingContainer.setOnClickListener(v -> {
            Intent toAdvanceTxSetting =
                    new Intent(requireActivity(), AdvanceTxSettingActivity.class);
            String nonce = mIsEIP1559 ? mTxInfo.txDataUnion.getEthTxData1559().baseData.nonce
                                      : mTxInfo.txDataUnion.getEthTxData().nonce;
            toAdvanceTxSetting.putExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_ID, mTxInfo.id)
                    .putExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_NONCE, nonce);
            startActivityForResult(toAdvanceTxSetting, START_ADVANCE_SETTING_ACTIVITY_CODE);
        });

        TextView editGasFee = view.findViewById(R.id.edit_gas_fee);
        editGasFee.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final Dialog dialog = new Dialog(getActivity());
                dialog.setContentView(R.layout.brave_wallet_edit_gas);
                dialog.show();
                mPreviousCheckedPriorityId = mCheckedPriorityId;

                LinearLayout gasPriceLayout = dialog.findViewById(R.id.gas_price_layout);
                LinearLayout gasLimitLayout = dialog.findViewById(R.id.gas_limit_layout);
                if (!mIsEIP1559) {
                    EditText gasFeeEdit = dialog.findViewById(R.id.gas_fee_edit);
                    gasFeeEdit.setText(String.format(Locale.getDefault(), "%.0f",
                            Utils.fromHexWei(
                                    mTxInfo.txDataUnion.getEthTxData1559().baseData.gasPrice, 9)));

                    EditText gasLimitEdit = dialog.findViewById(R.id.gas_limit_edit);
                    gasLimitEdit.setText(String.format(Locale.getDefault(), "%.0f",
                            Utils.fromHexGWeiToGWEI(
                                    mTxInfo.txDataUnion.getEthTxData1559().baseData.gasLimit)));
                } else {
                    TextView dialogTitle = dialog.findViewById(R.id.edit_gas_dialog_title);
                    dialogTitle.setText(
                            getResources().getString(R.string.wallet_max_priority_fee_title));
                    gasPriceLayout.setVisibility(View.GONE);
                    gasLimitLayout.setVisibility(View.GONE);
                    dialog.findViewById(R.id.max_priority_fee_msg).setVisibility(View.VISIBLE);
                    dialog.findViewById(R.id.max_priority_radio_group).setVisibility(View.VISIBLE);
                    RadioGroup radioGroup = dialog.findViewById(R.id.max_priority_radio_group);
                    radioGroup.clearCheck();
                    radioGroup.setOnCheckedChangeListener((group, checkedId) -> {
                        EthTxManagerProxy ethTxManagerProxy = getEthTxManagerProxy();
                        assert ethTxManagerProxy != null;
                        ethTxManagerProxy.getGasEstimation1559(estimation -> {
                            mTxInfo.txDataUnion.getEthTxData1559().gasEstimation = estimation;
                            mCheckedPriorityId = checkedId;
                            String gasLimit =
                                    mTxInfo.txDataUnion.getEthTxData1559().baseData.gasLimit;
                            String maxPriorityFeePerGas =
                                    mTxInfo.txDataUnion.getEthTxData1559().maxPriorityFeePerGas;
                            String maxFeePerGas =
                                    mTxInfo.txDataUnion.getEthTxData1559().maxFeePerGas;
                            TextView currentBaseFeeMsg =
                                    dialog.findViewById(R.id.current_base_fee_msg);
                            currentBaseFeeMsg.setVisibility(View.GONE);
                            LinearLayout gasAmountLimitLayout =
                                    dialog.findViewById(R.id.gas_amount_limit_layout);
                            gasAmountLimitLayout.setVisibility(View.GONE);
                            LinearLayout perGasTipLimitLayout =
                                    dialog.findViewById(R.id.per_gas_tip_limit_layout);
                            perGasTipLimitLayout.setVisibility(View.GONE);
                            LinearLayout perGasPriceLimitLayout =
                                    dialog.findViewById(R.id.per_gas_price_limit_layout);
                            perGasPriceLimitLayout.setVisibility(View.GONE);
                            if (mCheckedPriorityId == R.id.radio_low) {
                                maxPriorityFeePerGas =
                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                .gasEstimation.slowMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txDataUnion.getEthTxData1559()
                                                       .gasEstimation.slowMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_optimal) {
                                maxPriorityFeePerGas =
                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                .gasEstimation.avgMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txDataUnion.getEthTxData1559()
                                                       .gasEstimation.avgMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_high) {
                                maxPriorityFeePerGas =
                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                .gasEstimation.fastMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txDataUnion.getEthTxData1559()
                                                       .gasEstimation.fastMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_custom) {
                                currentBaseFeeMsg.setVisibility(View.VISIBLE);
                                currentBaseFeeMsg.setText(String.format(
                                        getResources().getString(R.string.wallet_current_base_fee),
                                        String.format(Locale.getDefault(), "%.0f",
                                                Utils.fromHexWei(
                                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                                .gasEstimation.baseFeePerGas,
                                                        9))));
                                gasAmountLimitLayout.setVisibility(View.VISIBLE);
                                EditText gasAmountLimitEdit =
                                        dialog.findViewById(R.id.gas_amount_limit_edit);
                                gasAmountLimitEdit.setText(
                                        String.format(Locale.getDefault(), "%.0f",
                                                Utils.fromHexGWeiToGWEI(
                                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                                .baseData.gasLimit)));
                                perGasTipLimitLayout.setVisibility(View.VISIBLE);
                                EditText perGasTipLimitEdit =
                                        dialog.findViewById(R.id.per_gas_tip_limit_edit);
                                perGasTipLimitEdit.setText(String.format(Locale.getDefault(),
                                        "%.0f", Utils.fromHexWei(maxPriorityFeePerGas, 9)));
                                perGasPriceLimitLayout.setVisibility(View.VISIBLE);
                                EditText perGasPriceLimitEdit =
                                        dialog.findViewById(R.id.per_gas_price_limit_edit);
                                perGasPriceLimitEdit.setText(String.format(Locale.getDefault(),
                                        "%.0f", Utils.fromHexWei(maxFeePerGas, 9)));
                                filterEIP1559TextWatcher.setDialog(dialog,
                                        String.format(Locale.getDefault(), "%.0f",
                                                Utils.fromHexWei(
                                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                                .gasEstimation.baseFeePerGas,
                                                        9)));
                                gasAmountLimitEdit.addTextChangedListener(filterEIP1559TextWatcher);
                                perGasTipLimitEdit.addTextChangedListener(filterEIP1559TextWatcher);
                                perGasPriceLimitEdit.addTextChangedListener(
                                        filterEIP1559TextWatcher);
                            }
                            fillMaxFee(dialog.findViewById(R.id.maximum_fee_msg), gasLimit,
                                    maxFeePerGas);
                        });
                    });
                    if (mCheckedPriorityId == -1) {
                        mCheckedPriorityId = R.id.radio_optimal;
                    }
                    radioGroup.check(mCheckedPriorityId);
                    dialog.findViewById(R.id.maximum_fee_msg).setVisibility(View.VISIBLE);
                }

                Button cancel = dialog.findViewById(R.id.cancel);
                cancel.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        mCheckedPriorityId = mPreviousCheckedPriorityId;
                        dialog.dismiss();
                    }
                });
                Button ok = dialog.findViewById(R.id.ok);
                ok.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        mPreviousCheckedPriorityId = mCheckedPriorityId;
                        EthTxManagerProxy ethTxManagerProxy = getEthTxManagerProxy();
                        assert ethTxManagerProxy != null;
                        if (ethTxManagerProxy == null) {
                            dialog.dismiss();

                            return;
                        }
                        if (!mIsEIP1559) {
                            EditText gasLimitEdit = dialog.findViewById(R.id.gas_limit_edit);
                            mTxInfo.txDataUnion.getEthTxData1559().baseData.gasLimit =
                                    Utils.toHexGWeiFromGWEI(gasLimitEdit.getText().toString());
                            EditText gasFeeEdit = dialog.findViewById(R.id.gas_fee_edit);
                            mTxInfo.txDataUnion.getEthTxData1559().baseData.gasPrice =
                                    Utils.toHexWei(gasFeeEdit.getText().toString(), 9);
                            ethTxManagerProxy.setGasPriceAndLimitForUnapprovedTransaction(
                                    mTxInfo.id,
                                    mTxInfo.txDataUnion.getEthTxData1559().baseData.gasPrice,
                                    mTxInfo.txDataUnion.getEthTxData1559().baseData.gasLimit,
                                    success -> {
                                        if (!success) {
                                            return;
                                        }
                                        setupView(view);
                                        dialog.dismiss();
                                    });
                        } else {
                            String gasLimit =
                                    mTxInfo.txDataUnion.getEthTxData1559().baseData.gasLimit;
                            String maxPriorityFeePerGas =
                                    mTxInfo.txDataUnion.getEthTxData1559().maxPriorityFeePerGas;
                            String maxFeePerGas =
                                    mTxInfo.txDataUnion.getEthTxData1559().maxFeePerGas;
                            if (mCheckedPriorityId == R.id.radio_low) {
                                maxPriorityFeePerGas =
                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                .gasEstimation.slowMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txDataUnion.getEthTxData1559()
                                                       .gasEstimation.slowMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_optimal) {
                                maxPriorityFeePerGas =
                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                .gasEstimation.avgMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txDataUnion.getEthTxData1559()
                                                       .gasEstimation.avgMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_high) {
                                maxPriorityFeePerGas =
                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                .gasEstimation.fastMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txDataUnion.getEthTxData1559()
                                                       .gasEstimation.fastMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_custom) {
                                EditText gasAmountLimitEdit =
                                        dialog.findViewById(R.id.gas_amount_limit_edit);
                                EditText perGasTipLimitEdit =
                                        dialog.findViewById(R.id.per_gas_tip_limit_edit);
                                EditText perGasPriceLimitEdit =
                                        dialog.findViewById(R.id.per_gas_price_limit_edit);
                                gasLimit = Utils.toHexGWeiFromGWEI(
                                        gasAmountLimitEdit.getText().toString());
                                maxPriorityFeePerGas =
                                        Utils.toHexWei(perGasTipLimitEdit.getText().toString(), 9);
                                maxFeePerGas = Utils.toHexWei(
                                        perGasPriceLimitEdit.getText().toString(), 9);
                            }
                            mTxInfo.txDataUnion.getEthTxData1559().baseData.gasLimit = gasLimit;
                            mTxInfo.txDataUnion.getEthTxData1559().maxPriorityFeePerGas =
                                    maxPriorityFeePerGas;
                            mTxInfo.txDataUnion.getEthTxData1559().maxFeePerGas = maxFeePerGas;
                            ethTxManagerProxy.setGasFeeAndLimitForUnapprovedTransaction(mTxInfo.id,
                                    maxPriorityFeePerGas, maxFeePerGas, gasLimit, success -> {
                                        if (!success) {
                                            return;
                                        }
                                        setupView(view);
                                        dialog.dismiss();
                                    });
                        }
                    }
                });
            }
        });

        return view;
    }

    private TextWatcherImpl filterEIP1559TextWatcher = new TextWatcherImpl();

    private class TextWatcherImpl implements TextWatcher {
        Dialog mDialog;
        String mBaseFeePerGas;
        boolean mIgnoreChange;

        public void setDialog(Dialog dialog, String baseFeePerGas) {
            mDialog = dialog;
            mBaseFeePerGas = baseFeePerGas;
            mIgnoreChange = false;
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            if (mIgnoreChange) {
                mIgnoreChange = false;

                return;
            }
            EditText gasAmountLimitEdit = mDialog.findViewById(R.id.gas_amount_limit_edit);
            EditText perGasTipLimitEdit = mDialog.findViewById(R.id.per_gas_tip_limit_edit);
            EditText perGasPriceLimitEdit = mDialog.findViewById(R.id.per_gas_price_limit_edit);
            String perGasPriceLimit = perGasPriceLimitEdit.getText().toString();
            if (perGasTipLimitEdit.hasFocus()) {
                try {
                    String perGasTipLimit = perGasTipLimitEdit.getText().toString();
                    perGasPriceLimit = String.format(Locale.getDefault(), "%.0f",
                            Double.valueOf(perGasTipLimit) + Double.valueOf(mBaseFeePerGas));
                    mIgnoreChange = true;
                    perGasPriceLimitEdit.setText(perGasPriceLimit);
                } catch (NumberFormatException exc) {
                }
            }
            if (gasAmountLimitEdit.hasFocus() || perGasTipLimitEdit.hasFocus()
                    || perGasPriceLimitEdit.hasFocus()) {
                fillMaxFee(mDialog.findViewById(R.id.maximum_fee_msg),
                        Utils.toHexGWeiFromGWEI(gasAmountLimitEdit.getText().toString()),
                        Utils.toHexWei(perGasPriceLimit, 9));
            }
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
    };

    private void fillMaxFee(TextView textView, String gasLimit, String maxFeePerGas) {
        double totalGas =
                Utils.fromHexWei(Utils.multiplyHexBN(gasLimit, maxFeePerGas), mChainDecimals);
        double price = totalGas * mEthRate;
        textView.setText(String.format(getResources().getString(R.string.wallet_maximum_fee),
                String.format(Locale.getDefault(), "%.2f", price),
                String.format(Locale.getDefault(), "%.8f", totalGas)));
    }

    private void setupView(View view) {
        TextView gasFeeAmount = view.findViewById(R.id.gas_fee_amount);
        final double totalGas = mIsEIP1559
                ? Utils.fromHexWei(Utils.multiplyHexBN(
                                           mTxInfo.txDataUnion.getEthTxData1559().baseData.gasLimit,
                                           mTxInfo.txDataUnion.getEthTxData1559().maxFeePerGas),
                        mChainDecimals)
                : Utils.fromHexWei(
                        Utils.multiplyHexBN(
                                mTxInfo.txDataUnion.getEthTxData1559().baseData.gasLimit,
                                mTxInfo.txDataUnion.getEthTxData1559().baseData.gasPrice),
                        mChainDecimals);
        gasFeeAmount.setText(
                String.format(getResources().getString(R.string.crypto_wallet_gas_fee_amount),
                        String.format(Locale.getDefault(), "%.8f", totalGas), mChainSymbol));
        String valueAsset = mTxInfo.txDataUnion.getEthTxData1559().baseData.value;
        if (mTxInfo.txType == TransactionType.ERC20_TRANSFER && mTxInfo.txArgs.length > 1) {
            valueAsset = mTxInfo.txArgs[1];
        }
        TextView totalAmount = view.findViewById(R.id.total_amount);
        totalAmount.setText(String.format(
                getResources().getString(R.string.crypto_wallet_total_amount),
                String.format(Locale.getDefault(), "%.8f", Utils.fromHexWei(valueAsset, mDecimals)),
                mAsset, String.format(Locale.getDefault(), "%.8f", totalGas), mChainSymbol));
        AssetRatioService assetRatioService = getAssetRatioService();
        if (assetRatioService != null) {
            String[] assets = {mChainSymbol.toLowerCase(Locale.getDefault())};
            String[] toCurr = {"usd"};
            assetRatioService.getPrice(
                    assets, toCurr, AssetPriceTimeframe.LIVE, (success, values) -> {
                        if (!success || values.length == 0) {
                            return;
                        }
                        mEthRate = Double.valueOf(values[0].price);
                        double totalPrice = totalGas * mEthRate;
                        TextView gasFeeAmountFiat = view.findViewById(R.id.gas_fee_amount_fiat);
                        gasFeeAmountFiat.setText(String.format(
                                getResources().getString(R.string.crypto_wallet_amount_fiat),
                                String.format(Locale.getDefault(), "%.2f", totalPrice)));
                        double totalAmountPlusGas = totalPrice + mTotalPrice;
                        TextView totalAmountFiat = view.findViewById(R.id.total_amount_fiat);
                        totalAmountFiat.setText(String.format(
                                getResources().getString(R.string.crypto_wallet_amount_fiat),
                                String.format(Locale.getDefault(), "%.2f", totalAmountPlusGas)));
                    });
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == START_ADVANCE_SETTING_ACTIVITY_CODE
                && resultCode == Activity.RESULT_OK) {
            String nonce =
                    data.getStringExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_RESULT_NONCE);
            if (mIsEIP1559) {
                mTxInfo.txDataUnion.getEthTxData1559().baseData.nonce = nonce;
            } else {
                mTxInfo.txDataUnion.getEthTxData().nonce = nonce;
            }
        }
    }
}
