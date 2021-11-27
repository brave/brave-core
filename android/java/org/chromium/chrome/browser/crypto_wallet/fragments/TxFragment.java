/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.fragments;

import android.app.Activity;
import android.app.Dialog;
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
import org.chromium.brave_wallet.mojom.AssetRatioController;
import org.chromium.brave_wallet.mojom.EthTxController;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.brave_wallet.mojom.TxData;
import org.chromium.brave_wallet.mojom.TxData1559;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BuySendSwapActivity;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;

import java.util.Locale;

public class TxFragment extends Fragment {
    private TransactionInfo mTxInfo;
    private String mAsset;
    private double mTotalPrice;
    private boolean mIsEIP1559;
    private int mCheckedPriorityId;
    private int mPreviousCheckedPriorityId;
    private double mEthRate;

    public static TxFragment newInstance(TransactionInfo txInfo, String asset, double totalPrice) {
        return new TxFragment(txInfo, asset, totalPrice);
    }

    private AssetRatioController getAssetRatioController() {
        Activity activity = getActivity();
        if (activity instanceof BuySendSwapActivity) {
            return ((BuySendSwapActivity) activity).getAssetRatioController();
        } else if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getAssetRatioController();
        }

        return null;
    }

    private EthTxController getEthTxController() {
        Activity activity = getActivity();
        if (activity instanceof BuySendSwapActivity) {
            return ((BuySendSwapActivity) activity).getEthTxController();
        } else if (activity instanceof BraveWalletActivity) {
            return ((BraveWalletActivity) activity).getEthTxController();
        }

        return null;
    }

    private TxFragment(TransactionInfo txInfo, String asset, double totalPrice) {
        mTxInfo = txInfo;
        mAsset = asset;
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
        mIsEIP1559 = !mTxInfo.txData.maxPriorityFeePerGas.isEmpty()
                && !mTxInfo.txData.maxFeePerGas.isEmpty();

        setupView(view);

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
                            Utils.fromHexWei(mTxInfo.txData.baseData.gasPrice, 9)));

                    EditText gasLimitEdit = dialog.findViewById(R.id.gas_limit_edit);
                    gasLimitEdit.setText(String.format(Locale.getDefault(), "%.0f",
                            Utils.fromHexGWeiToGWEI(mTxInfo.txData.baseData.gasLimit)));
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
                        AssetRatioController assetRatioController = getAssetRatioController();
                        assert assetRatioController != null;
                        assetRatioController.getGasOracle(estimation -> {
                            mTxInfo.txData.gasEstimation = estimation;
                            mCheckedPriorityId = checkedId;
                            String gasLimit = mTxInfo.txData.baseData.gasLimit;
                            String maxPriorityFeePerGas = mTxInfo.txData.maxPriorityFeePerGas;
                            String maxFeePerGas = mTxInfo.txData.maxFeePerGas;
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
                                        mTxInfo.txData.gasEstimation.slowMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txData.gasEstimation.slowMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_optimal) {
                                maxPriorityFeePerGas =
                                        mTxInfo.txData.gasEstimation.avgMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txData.gasEstimation.avgMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_high) {
                                maxPriorityFeePerGas =
                                        mTxInfo.txData.gasEstimation.fastMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txData.gasEstimation.fastMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_custom) {
                                currentBaseFeeMsg.setVisibility(View.VISIBLE);
                                currentBaseFeeMsg.setText(String.format(
                                        getResources().getString(R.string.wallet_current_base_fee),
                                        String.format(Locale.getDefault(), "%.0f",
                                                Utils.fromHexWei(
                                                        mTxInfo.txData.gasEstimation.baseFeePerGas,
                                                        9))));
                                gasAmountLimitLayout.setVisibility(View.VISIBLE);
                                EditText gasAmountLimitEdit =
                                        dialog.findViewById(R.id.gas_amount_limit_edit);
                                gasAmountLimitEdit.setText(String.format(Locale.getDefault(),
                                        "%.0f",
                                        Utils.fromHexGWeiToGWEI(mTxInfo.txData.baseData.gasLimit)));
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
                                                        mTxInfo.txData.gasEstimation.baseFeePerGas,
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
                        EthTxController ethTxController = getEthTxController();
                        assert ethTxController != null;
                        if (ethTxController == null) {
                            dialog.dismiss();

                            return;
                        }
                        if (!mIsEIP1559) {
                            EditText gasLimitEdit = dialog.findViewById(R.id.gas_limit_edit);
                            mTxInfo.txData.baseData.gasLimit =
                                    Utils.toHexWeiFromGWEI(gasLimitEdit.getText().toString());
                            EditText gasFeeEdit = dialog.findViewById(R.id.gas_fee_edit);
                            mTxInfo.txData.baseData.gasPrice =
                                    Utils.toHexWei(gasFeeEdit.getText().toString(), 9);
                            ethTxController.setGasPriceAndLimitForUnapprovedTransaction(mTxInfo.id,
                                    mTxInfo.txData.baseData.gasPrice,
                                    mTxInfo.txData.baseData.gasLimit, success -> {
                                        if (!success) {
                                            return;
                                        }
                                        setupView(view);
                                        dialog.dismiss();
                                    });
                        } else {
                            String gasLimit = mTxInfo.txData.baseData.gasLimit;
                            String maxPriorityFeePerGas = mTxInfo.txData.maxPriorityFeePerGas;
                            String maxFeePerGas = mTxInfo.txData.maxFeePerGas;
                            if (mCheckedPriorityId == R.id.radio_low) {
                                maxPriorityFeePerGas =
                                        mTxInfo.txData.gasEstimation.slowMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txData.gasEstimation.slowMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_optimal) {
                                maxPriorityFeePerGas =
                                        mTxInfo.txData.gasEstimation.avgMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txData.gasEstimation.avgMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_high) {
                                maxPriorityFeePerGas =
                                        mTxInfo.txData.gasEstimation.fastMaxPriorityFeePerGas;
                                maxFeePerGas = mTxInfo.txData.gasEstimation.fastMaxFeePerGas;
                            } else if (mCheckedPriorityId == R.id.radio_custom) {
                                EditText gasAmountLimitEdit =
                                        dialog.findViewById(R.id.gas_amount_limit_edit);
                                EditText perGasTipLimitEdit =
                                        dialog.findViewById(R.id.per_gas_tip_limit_edit);
                                EditText perGasPriceLimitEdit =
                                        dialog.findViewById(R.id.per_gas_price_limit_edit);
                                gasLimit = Utils.toHexWeiFromGWEI(
                                        gasAmountLimitEdit.getText().toString());
                                maxPriorityFeePerGas =
                                        Utils.toHexWei(perGasTipLimitEdit.getText().toString(), 9);
                                maxFeePerGas = Utils.toHexWei(
                                        perGasPriceLimitEdit.getText().toString(), 9);
                            }
                            mTxInfo.txData.baseData.gasLimit = gasLimit;
                            mTxInfo.txData.maxPriorityFeePerGas = maxPriorityFeePerGas;
                            mTxInfo.txData.maxFeePerGas = maxFeePerGas;
                            ethTxController.setGasFeeAndLimitForUnapprovedTransaction(mTxInfo.id,
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
                        Utils.toHexWeiFromGWEI(gasAmountLimitEdit.getText().toString()),
                        Utils.toHexWei(perGasPriceLimit, 9));
            }
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
    };

    private void fillMaxFee(TextView textView, String gasLimit, String maxFeePerGas) {
        double totalGas = Utils.fromHexWei(Utils.multiplyHexBN(gasLimit, maxFeePerGas), 18);
        double price = totalGas * mEthRate;
        textView.setText(String.format(getResources().getString(R.string.wallet_maximum_fee),
                String.format(Locale.getDefault(), "%.2f", price),
                String.format(Locale.getDefault(), "%.8f", totalGas)));
    }

    private void setupView(View view) {
        TextView gasFeeAmount = view.findViewById(R.id.gas_fee_amount);
        final double totalGas = mIsEIP1559
                ? Utils.fromHexWei(Utils.multiplyHexBN(mTxInfo.txData.baseData.gasLimit,
                                           mTxInfo.txData.maxFeePerGas),
                        18)
                : Utils.fromHexWei(Utils.multiplyHexBN(mTxInfo.txData.baseData.gasLimit,
                                           mTxInfo.txData.baseData.gasPrice),
                        18);
        gasFeeAmount.setText(
                String.format(getResources().getString(R.string.crypto_wallet_gas_fee_amount),
                        String.format(Locale.getDefault(), "%.8f", totalGas)));
        String valueAsset = mTxInfo.txData.baseData.value;
        if (mTxInfo.txType == TransactionType.ERC20_TRANSFER && mTxInfo.txArgs.length > 1) {
            valueAsset = mTxInfo.txArgs[1];
        }
        TextView totalAmount = view.findViewById(R.id.total_amount);
        totalAmount.setText(String.format(
                getResources().getString(R.string.crypto_wallet_total_amount),
                String.format(Locale.getDefault(), "%.8f", Utils.fromHexWei(valueAsset, 18)),
                mAsset, String.format(Locale.getDefault(), "%.8f", totalGas)));
        AssetRatioController assetRatioController = getAssetRatioController();
        if (assetRatioController != null) {
            String[] assets = {"eth"};
            String[] toCurr = {"usd"};
            assetRatioController.getPrice(
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
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
    }
}
