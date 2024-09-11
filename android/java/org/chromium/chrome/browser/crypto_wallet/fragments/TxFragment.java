/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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

import org.chromium.brave_wallet.mojom.AccountInfo;
import org.chromium.brave_wallet.mojom.BlockchainToken;
import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.EthTxManagerProxy;
import org.chromium.brave_wallet.mojom.NetworkInfo;
import org.chromium.brave_wallet.mojom.SolanaSendTransactionOptions;
import org.chromium.brave_wallet.mojom.SolanaTxData;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.crypto_wallet.activities.AdvanceTxSettingActivity;
import org.chromium.chrome.browser.crypto_wallet.activities.BraveWalletBaseActivity;
import org.chromium.chrome.browser.crypto_wallet.presenters.Amount;
import org.chromium.chrome.browser.crypto_wallet.util.AndroidUtils;
import org.chromium.chrome.browser.crypto_wallet.util.ParsedTransaction;
import org.chromium.chrome.browser.crypto_wallet.util.ParsedTransactionFees;
import org.chromium.chrome.browser.crypto_wallet.util.TransactionUtils;
import org.chromium.chrome.browser.crypto_wallet.util.Utils;
import org.chromium.chrome.browser.crypto_wallet.util.WalletConstants;

import java.util.HashMap;
import java.util.Locale;

public class TxFragment extends Fragment {
    private TransactionInfo mTxInfo;
    private ParsedTransaction mParsedTx;
    private NetworkInfo mTxNetwork;
    private AccountInfo[] mAccounts;
    private HashMap<String, Double> mAssetPrices;
    private BlockchainToken[] mFullTokenList;
    private HashMap<String, Double> mNativeAssetsBalances;
    private HashMap<String, HashMap<String, Double>> mBlockchainTokensBalances;
    private int mCheckedPriorityId;
    private int mPreviousCheckedPriorityId;
    private long mSolanaEstimatedTxFee;

    // mUpdateTxObjectManually is used to detect do we need to update dialog values
    // manually after we change gas for example or do we have it updated automatically
    // using observers. We use observers on DApps related executions, but wallet screens
    // don't use them. It would be good to eventually migrate to observers everywhere.
    private boolean mUpdateTxObjectManually;
    public static final int START_ADVANCE_SETTING_ACTIVITY_CODE = 0;
    private boolean mIsSolanaInstruction;

    public static TxFragment newInstance(
            TransactionInfo txInfo,
            NetworkInfo txNetwork,
            AccountInfo[] accounts,
            HashMap<String, Double> assetPrices,
            BlockchainToken[] fullTokenList,
            HashMap<String, Double> nativeAssetsBalances,
            HashMap<String, HashMap<String, Double>> blockchainTokensBalances,
            boolean updateTxObjectManually,
            long solanaEstimatedTxFee) {
        return new TxFragment(
                txInfo,
                txNetwork,
                accounts,
                assetPrices,
                fullTokenList,
                nativeAssetsBalances,
                blockchainTokensBalances,
                updateTxObjectManually,
                solanaEstimatedTxFee);
    }

    private EthTxManagerProxy getEthTxManagerProxy() {
        Activity activity = getActivity();
        if (activity instanceof BraveWalletBaseActivity) {
            return ((BraveWalletBaseActivity) activity).getEthTxManagerProxy();
        }
        return null;
    }

    private TxFragment(
            TransactionInfo txInfo,
            NetworkInfo txNetwork,
            AccountInfo[] accounts,
            HashMap<String, Double> assetPrices,
            BlockchainToken[] fullTokenList,
            HashMap<String, Double> nativeAssetsBalances,
            HashMap<String, HashMap<String, Double>> blockchainTokensBalances,
            boolean updateTxObjectManually,
            long solanaEstimatedTxFee) {
        mTxInfo = txInfo;
        mTxNetwork = txNetwork;
        mAccounts = accounts;
        mAssetPrices = assetPrices;
        mFullTokenList = fullTokenList;
        mNativeAssetsBalances = nativeAssetsBalances;
        mBlockchainTokensBalances = blockchainTokensBalances;
        mCheckedPriorityId = -1;
        mPreviousCheckedPriorityId = -1;
        mUpdateTxObjectManually = updateTxObjectManually;
        mSolanaEstimatedTxFee = solanaEstimatedTxFee;
        mIsSolanaInstruction = TransactionUtils.isSolanaTx(txInfo);
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Nullable
    @Override
    public View onCreateView(
            @NonNull LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.fragment_transaction, container, false);

        setupView(view);

        View advanceSettingContainer = view.findViewById(R.id.fragment_tx_tv_advance_setting);
        advanceSettingContainer.setVisibility(
                isAdvanceSettingEnabled(mTxNetwork) ? View.VISIBLE : View.GONE);

        advanceSettingContainer.setOnClickListener(
                v -> {
                    Intent toAdvanceTxSetting =
                            new Intent(requireActivity(), AdvanceTxSettingActivity.class);
                    String nonce = mParsedTx.getNonce();
                    toAdvanceTxSetting
                            .putExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_ID, mTxInfo.id)
                            .putExtra(
                                    WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_CHAIN_ID,
                                    mTxInfo.chainId)
                            .putExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_TX_NONCE, nonce);
                    startActivityForResult(toAdvanceTxSetting, START_ADVANCE_SETTING_ACTIVITY_CODE);
                });

        TextView editGasFee = view.findViewById(R.id.edit_gas_fee);
        editGasFee.setVisibility(isEditTxEnabled(mTxNetwork) ? View.VISIBLE : View.INVISIBLE);
        editGasFee.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        final Dialog dialog = new Dialog(getActivity());
                        dialog.setContentView(R.layout.brave_wallet_edit_gas);
                        dialog.show();
                        mPreviousCheckedPriorityId = mCheckedPriorityId;

                        LinearLayout gasPriceLayout = dialog.findViewById(R.id.gas_price_layout);
                        LinearLayout gasLimitLayout = dialog.findViewById(R.id.gas_limit_layout);
                        if (!mParsedTx.getIsEIP1559Transaction()) {
                            EditText gasFeeEdit = dialog.findViewById(R.id.gas_fee_edit);
                            gasFeeEdit.setText(
                                    String.format(
                                            Locale.getDefault(),
                                            "%.0f",
                                            Utils.fromHexWei(mParsedTx.getGasPrice(), 9)));

                            EditText gasLimitEdit = dialog.findViewById(R.id.gas_limit_edit);
                            gasLimitEdit.setText(
                                    String.format(
                                            Locale.getDefault(),
                                            "%.0f",
                                            Utils.fromHexGWeiToGWEI(mParsedTx.getGasLimit())));
                        } else {
                            TextView dialogTitle = dialog.findViewById(R.id.edit_gas_dialog_title);
                            dialogTitle.setText(
                                    getResources()
                                            .getString(R.string.wallet_max_priority_fee_title));
                            gasPriceLayout.setVisibility(View.GONE);
                            gasLimitLayout.setVisibility(View.GONE);
                            dialog.findViewById(R.id.max_priority_fee_msg)
                                    .setVisibility(View.VISIBLE);
                            dialog.findViewById(R.id.max_priority_radio_group)
                                    .setVisibility(View.VISIBLE);
                            RadioGroup radioGroup =
                                    dialog.findViewById(R.id.max_priority_radio_group);
                            radioGroup.clearCheck();
                            radioGroup.setOnCheckedChangeListener(
                                    (group, checkedId) -> {
                                        EthTxManagerProxy ethTxManagerProxy =
                                                getEthTxManagerProxy();
                                        assert ethTxManagerProxy != null;
                                        ethTxManagerProxy.getGasEstimation1559(
                                                mTxInfo.chainId,
                                                estimation -> {
                                                    mTxInfo.txDataUnion.getEthTxData1559()
                                                                    .gasEstimation =
                                                            estimation;
                                                    mCheckedPriorityId = checkedId;
                                                    String maxPriorityFeePerGas =
                                                            mParsedTx.getMaxPriorityFeePerGas();
                                                    String maxFeePerGas =
                                                            mParsedTx.getMaxFeePerGas();
                                                    TextView currentBaseFeeMsg =
                                                            dialog.findViewById(
                                                                    R.id.current_base_fee_msg);
                                                    currentBaseFeeMsg.setVisibility(View.GONE);
                                                    LinearLayout gasAmountLimitLayout =
                                                            dialog.findViewById(
                                                                    R.id.gas_amount_limit_layout);
                                                    gasAmountLimitLayout.setVisibility(View.GONE);
                                                    LinearLayout perGasTipLimitLayout =
                                                            dialog.findViewById(
                                                                    R.id.per_gas_tip_limit_layout);
                                                    perGasTipLimitLayout.setVisibility(View.GONE);
                                                    LinearLayout perGasPriceLimitLayout =
                                                            dialog.findViewById(
                                                                    R.id
                                                                            .per_gas_price_limit_layout);
                                                    perGasPriceLimitLayout.setVisibility(View.GONE);
                                                    if (mCheckedPriorityId == R.id.radio_low) {
                                                        maxPriorityFeePerGas =
                                                                mTxInfo.txDataUnion
                                                                        .getEthTxData1559()
                                                                        .gasEstimation
                                                                        .slowMaxPriorityFeePerGas;
                                                        maxFeePerGas =
                                                                mTxInfo.txDataUnion
                                                                        .getEthTxData1559()
                                                                        .gasEstimation
                                                                        .slowMaxFeePerGas;
                                                    } else if (mCheckedPriorityId
                                                            == R.id.radio_optimal) {
                                                        maxPriorityFeePerGas =
                                                                mTxInfo.txDataUnion
                                                                        .getEthTxData1559()
                                                                        .gasEstimation
                                                                        .avgMaxPriorityFeePerGas;
                                                        maxFeePerGas =
                                                                mTxInfo.txDataUnion
                                                                        .getEthTxData1559()
                                                                        .gasEstimation
                                                                        .avgMaxFeePerGas;
                                                    } else if (mCheckedPriorityId
                                                            == R.id.radio_high) {
                                                        maxPriorityFeePerGas =
                                                                mTxInfo.txDataUnion
                                                                        .getEthTxData1559()
                                                                        .gasEstimation
                                                                        .fastMaxPriorityFeePerGas;
                                                        maxFeePerGas =
                                                                mTxInfo.txDataUnion
                                                                        .getEthTxData1559()
                                                                        .gasEstimation
                                                                        .fastMaxFeePerGas;
                                                    } else if (mCheckedPriorityId
                                                            == R.id.radio_custom) {
                                                        currentBaseFeeMsg.setVisibility(
                                                                View.VISIBLE);
                                                        currentBaseFeeMsg.setText(
                                                                String.format(
                                                                        getResources()
                                                                                .getString(
                                                                                        R.string
                                                                                                .wallet_current_base_fee),
                                                                        String.format(
                                                                                Locale.getDefault(),
                                                                                "%.0f",
                                                                                Utils.fromHexWei(
                                                                                        mTxInfo
                                                                                                .txDataUnion
                                                                                                .getEthTxData1559()
                                                                                                .gasEstimation
                                                                                                .baseFeePerGas,
                                                                                        9))));
                                                        gasAmountLimitLayout.setVisibility(
                                                                View.VISIBLE);
                                                        EditText gasAmountLimitEdit =
                                                                dialog.findViewById(
                                                                        R.id.gas_amount_limit_edit);
                                                        gasAmountLimitEdit.setText(
                                                                String.format(
                                                                        Locale.getDefault(),
                                                                        "%.0f",
                                                                        Utils.fromHexGWeiToGWEI(
                                                                                mParsedTx
                                                                                        .getGasLimit())));
                                                        perGasTipLimitLayout.setVisibility(
                                                                View.VISIBLE);
                                                        EditText perGasTipLimitEdit =
                                                                dialog.findViewById(
                                                                        R.id
                                                                                .per_gas_tip_limit_edit);
                                                        perGasTipLimitEdit.setText(
                                                                String.format(
                                                                        Locale.getDefault(),
                                                                        "%.0f",
                                                                        Utils.fromHexWei(
                                                                                maxPriorityFeePerGas,
                                                                                9)));
                                                        perGasPriceLimitLayout.setVisibility(
                                                                View.VISIBLE);
                                                        EditText perGasPriceLimitEdit =
                                                                dialog.findViewById(
                                                                        R.id
                                                                                .per_gas_price_limit_edit);
                                                        perGasPriceLimitEdit.setText(
                                                                String.format(
                                                                        Locale.getDefault(),
                                                                        "%.0f",
                                                                        Utils.fromHexWei(
                                                                                maxFeePerGas, 9)));
                                                        filterEIP1559TextWatcher.setDialog(
                                                                dialog,
                                                                String.format(
                                                                        Locale.getDefault(),
                                                                        "%.0f",
                                                                        Utils.fromHexWei(
                                                                                mTxInfo.txDataUnion
                                                                                        .getEthTxData1559()
                                                                                        .gasEstimation
                                                                                        .baseFeePerGas,
                                                                                9)));
                                                        gasAmountLimitEdit.addTextChangedListener(
                                                                filterEIP1559TextWatcher);
                                                        perGasTipLimitEdit.addTextChangedListener(
                                                                filterEIP1559TextWatcher);
                                                        perGasPriceLimitEdit.addTextChangedListener(
                                                                filterEIP1559TextWatcher);
                                                    }
                                                    fillMaxFee(
                                                            dialog.findViewById(
                                                                    R.id.maximum_fee_msg),
                                                            mParsedTx.getGasLimit(),
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
                        cancel.setOnClickListener(
                                new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        mCheckedPriorityId = mPreviousCheckedPriorityId;
                                        dialog.dismiss();
                                    }
                                });
                        Button ok = dialog.findViewById(R.id.ok);
                        ok.setOnClickListener(
                                new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        mPreviousCheckedPriorityId = mCheckedPriorityId;
                                        EthTxManagerProxy ethTxManagerProxy =
                                                getEthTxManagerProxy();
                                        assert ethTxManagerProxy != null;
                                        if (ethTxManagerProxy == null) {
                                            dialog.dismiss();

                                            return;
                                        }
                                        if (!mParsedTx.getIsEIP1559Transaction()) {
                                            EditText gasLimitEdit =
                                                    dialog.findViewById(R.id.gas_limit_edit);
                                            mTxInfo.txDataUnion.getEthTxData1559()
                                                            .baseData
                                                            .gasLimit =
                                                    Utils.toHexGWeiFromGWEI(
                                                            gasLimitEdit.getText().toString());
                                            EditText gasFeeEdit =
                                                    dialog.findViewById(R.id.gas_fee_edit);
                                            mTxInfo.txDataUnion.getEthTxData1559()
                                                            .baseData
                                                            .gasPrice =
                                                    Utils.toHexWei(
                                                            gasFeeEdit.getText().toString(), 9);
                                            ethTxManagerProxy
                                                    .setGasPriceAndLimitForUnapprovedTransaction(
                                                            mTxInfo.chainId,
                                                            mTxInfo.id,
                                                            mTxInfo.txDataUnion.getEthTxData1559()
                                                                    .baseData
                                                                    .gasPrice,
                                                            mTxInfo.txDataUnion.getEthTxData1559()
                                                                    .baseData
                                                                    .gasLimit,
                                                            success -> {
                                                                if (!success) {
                                                                    return;
                                                                }
                                                                if (mUpdateTxObjectManually) {
                                                                    setupView(view);
                                                                }
                                                                dialog.dismiss();
                                                            });
                                        } else {
                                            String gasLimit = mParsedTx.getGasLimit();
                                            String maxPriorityFeePerGas =
                                                    mParsedTx.getMaxPriorityFeePerGas();
                                            String maxFeePerGas = mParsedTx.getMaxFeePerGas();
                                            if (mCheckedPriorityId == R.id.radio_low) {
                                                maxPriorityFeePerGas =
                                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                                .gasEstimation
                                                                .slowMaxPriorityFeePerGas;
                                                maxFeePerGas =
                                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                                .gasEstimation
                                                                .slowMaxFeePerGas;
                                            } else if (mCheckedPriorityId == R.id.radio_optimal) {
                                                maxPriorityFeePerGas =
                                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                                .gasEstimation
                                                                .avgMaxPriorityFeePerGas;
                                                maxFeePerGas =
                                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                                .gasEstimation
                                                                .avgMaxFeePerGas;
                                            } else if (mCheckedPriorityId == R.id.radio_high) {
                                                maxPriorityFeePerGas =
                                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                                .gasEstimation
                                                                .fastMaxPriorityFeePerGas;
                                                maxFeePerGas =
                                                        mTxInfo.txDataUnion.getEthTxData1559()
                                                                .gasEstimation
                                                                .fastMaxFeePerGas;
                                            } else if (mCheckedPriorityId == R.id.radio_custom) {
                                                EditText gasAmountLimitEdit =
                                                        dialog.findViewById(
                                                                R.id.gas_amount_limit_edit);
                                                EditText perGasTipLimitEdit =
                                                        dialog.findViewById(
                                                                R.id.per_gas_tip_limit_edit);
                                                EditText perGasPriceLimitEdit =
                                                        dialog.findViewById(
                                                                R.id.per_gas_price_limit_edit);
                                                gasLimit =
                                                        Utils.toHexGWeiFromGWEI(
                                                                gasAmountLimitEdit
                                                                        .getText()
                                                                        .toString());
                                                maxPriorityFeePerGas =
                                                        Utils.toHexWei(
                                                                perGasTipLimitEdit
                                                                        .getText()
                                                                        .toString(),
                                                                9);
                                                maxFeePerGas =
                                                        Utils.toHexWei(
                                                                perGasPriceLimitEdit
                                                                        .getText()
                                                                        .toString(),
                                                                9);
                                            }
                                            mTxInfo.txDataUnion.getEthTxData1559()
                                                            .baseData
                                                            .gasLimit =
                                                    gasLimit;
                                            mTxInfo.txDataUnion.getEthTxData1559()
                                                            .maxPriorityFeePerGas =
                                                    maxPriorityFeePerGas;
                                            mTxInfo.txDataUnion.getEthTxData1559().maxFeePerGas =
                                                    maxFeePerGas;
                                            ethTxManagerProxy
                                                    .setGasFeeAndLimitForUnapprovedTransaction(
                                                            mTxInfo.chainId,
                                                            mTxInfo.id,
                                                            maxPriorityFeePerGas,
                                                            maxFeePerGas,
                                                            gasLimit,
                                                            success -> {
                                                                if (!success) {
                                                                    return;
                                                                }
                                                                if (mUpdateTxObjectManually) {
                                                                    setupView(view);
                                                                }
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
                    perGasPriceLimit =
                            String.format(
                                    Locale.getDefault(),
                                    "%.0f",
                                    Double.valueOf(perGasTipLimit)
                                            + Double.valueOf(mBaseFeePerGas));
                    mIgnoreChange = true;
                    perGasPriceLimitEdit.setText(perGasPriceLimit);
                } catch (NumberFormatException exc) {
                }
            }
            if (gasAmountLimitEdit.hasFocus()
                    || perGasTipLimitEdit.hasFocus()
                    || perGasPriceLimitEdit.hasFocus()) {
                fillMaxFee(
                        mDialog.findViewById(R.id.maximum_fee_msg),
                        Utils.toHexGWeiFromGWEI(gasAmountLimitEdit.getText().toString()),
                        Utils.toHexWei(perGasPriceLimit, 9));
            }
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

        @Override
        public void afterTextChanged(Editable s) {}
    }
    ;

    private void fillMaxFee(TextView textView, String gasLimit, String maxFeePerGas) {
        final double[] gasFeeArr =
                ParsedTransactionFees.calcGasFee(
                        mTxNetwork,
                        Utils.getOrDefault(
                                mAssetPrices, mTxNetwork.symbol.toLowerCase(Locale.ENGLISH), 0.0d),
                        true,
                        gasLimit,
                        "0",
                        maxFeePerGas,
                        false,
                        mSolanaEstimatedTxFee);
        textView.setText(
                String.format(
                        getResources().getString(R.string.wallet_maximum_fee),
                        String.format(Locale.getDefault(), "%.2f", gasFeeArr[1]),
                        String.format(Locale.getDefault(), "%.8f", gasFeeArr[0])));
    }

    private void setupView(View view) {
        // Re-parse transaction for mUpdateTxObjectManually
        // TODO(sergz): Not really sure do we need to re-parse here as we parse it in the
        // parent fragment
        mParsedTx =
                ParsedTransaction.parseTransaction(
                        mTxInfo,
                        mTxNetwork,
                        mAccounts,
                        mAssetPrices,
                        mSolanaEstimatedTxFee,
                        mFullTokenList,
                        mNativeAssetsBalances,
                        mBlockchainTokensBalances);

        if (mIsSolanaInstruction) {
            TextView gasTxTv = view.findViewById(R.id.frag_tx_tv_gas);
            TextView totalHeading = view.findViewById(R.id.frag_tx_tv_total_heading);

            gasTxTv.setText(R.string.brave_wallet_allow_spend_transaction_fee);
            totalHeading.setText(R.string.brave_wallet_confirm_transaction_amount_fee);
            SolanaTxData solanaTxData = TransactionUtils.safeSolData(mTxInfo.txDataUnion);
            if (solanaTxData != null && solanaTxData.sendOptions != null) {
                SolanaSendTransactionOptions sendTxOptions = solanaTxData.sendOptions;
                LinearLayout sendOptionsLinearLayout =
                        view.findViewById(R.id.frag_tx_ll_send_options);
                if (sendTxOptions.maxRetries != null) {
                    TextView tvLabel = AndroidUtils.makeHeaderTv(requireContext());
                    TextView tvVal = AndroidUtils.makeSubHeaderTv(requireContext());
                    tvLabel.setText(R.string.brave_wallet_solana_max_retries);
                    tvVal.setText(String.valueOf(sendTxOptions.maxRetries.maxRetries));
                    sendOptionsLinearLayout.addView(tvLabel);
                    sendOptionsLinearLayout.addView(tvVal);
                }
                if (sendTxOptions.preflightCommitment != null) {
                    TextView tvLabel = AndroidUtils.makeHeaderTv(requireContext());
                    TextView tvVal = AndroidUtils.makeSubHeaderTv(requireContext());
                    tvLabel.setText(R.string.brave_wallet_solana_preflight_commitment);
                    tvVal.setText(sendTxOptions.preflightCommitment);
                    sendOptionsLinearLayout.addView(tvLabel);
                    sendOptionsLinearLayout.addView(tvVal);
                }
                if (sendTxOptions.skipPreflight != null) {
                    TextView tvLabel = AndroidUtils.makeHeaderTv(requireContext());
                    TextView tvVal = AndroidUtils.makeSubHeaderTv(requireContext());
                    tvLabel.setText(R.string.brave_wallet_solana_skip_preflight);
                    tvVal.setText(String.valueOf(sendTxOptions.skipPreflight.skipPreflight));
                    sendOptionsLinearLayout.addView(tvLabel);
                    sendOptionsLinearLayout.addView(tvVal);
                }
            }
        }
        TextView gasFeeAmount = view.findViewById(R.id.gas_fee_amount);
        final double totalGas = mParsedTx.getGasFee();
        String symbol = mParsedTx.getSymbol() == null ? mParsedTx.getSymbol() : mTxNetwork.symbol;
        gasFeeAmount.setText(
                String.format(
                        getResources().getString(R.string.crypto_wallet_gas_fee_amount),
                        String.format(Locale.getDefault(), "%.8f", totalGas),
                        symbol));

        String valueAssetText = mParsedTx.formatValueToDisplay();
        TextView totalAmount = view.findViewById(R.id.total_amount);
        totalAmount.setText(
                String.format(
                        getResources().getString(R.string.crypto_wallet_total_amount),
                        valueAssetText,
                        mParsedTx.getSymbol(),
                        String.format(Locale.getDefault(), "%.8f", totalGas),
                        symbol));

        TextView gasFeeAmountFiat = view.findViewById(R.id.gas_fee_amount_fiat);
        gasFeeAmountFiat.setText(
                String.format(
                        getResources().getString(R.string.crypto_wallet_amount_fiat),
                        new Amount(mParsedTx.getFiatTotal()).toStringFormat()));
        TextView totalAmountFiat = view.findViewById(R.id.total_amount_fiat);
        totalAmountFiat.setText(
                String.format(
                        getResources().getString(R.string.crypto_wallet_amount_fiat),
                        new Amount(mParsedTx.getFiatTotal()).toStringFormat()));
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == START_ADVANCE_SETTING_ACTIVITY_CODE
                && resultCode == Activity.RESULT_OK) {
            String nonce =
                    data.getStringExtra(WalletConstants.ADVANCE_TX_SETTING_INTENT_RESULT_NONCE);
            if (mParsedTx.getIsEIP1559Transaction()) {
                mTxInfo.txDataUnion.getEthTxData1559().baseData.nonce = nonce;
            } else {
                mTxInfo.txDataUnion.getEthTxData().nonce = nonce;
            }
        }
    }

    private boolean isEditTxEnabled(NetworkInfo txNetwork) {
        return txNetwork.coin == CoinType.ETH;
    }

    private boolean isAdvanceSettingEnabled(NetworkInfo txNetwork) {
        return isEditTxEnabled(txNetwork);
    }
}
