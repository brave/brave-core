/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.crypto_wallet.util;

import android.content.Context;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;

import org.chromium.brave_wallet.mojom.CoinType;
import org.chromium.brave_wallet.mojom.SignSolTransactionsRequest;
import org.chromium.brave_wallet.mojom.SolanaSystemInstruction;
import org.chromium.brave_wallet.mojom.SolanaTokenInstruction;
import org.chromium.brave_wallet.mojom.SolanaTxData;
import org.chromium.brave_wallet.mojom.TransactionInfo;
import org.chromium.brave_wallet.mojom.TransactionType;
import org.chromium.brave_wallet.mojom.TxDataUnion;
import org.chromium.chrome.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class TransactionUtils {
    @CoinType.EnumType
    public static int getCoinFromTxDataUnion(@NonNull final TxDataUnion txDataUnion) {
        if (txDataUnion.which() == TxDataUnion.Tag.BtcTxData) {
            return CoinType.BTC;
        } else if (txDataUnion.which() == TxDataUnion.Tag.ZecTxData) {
            return CoinType.ZEC;
        } else if (txDataUnion.which() == TxDataUnion.Tag.FilTxData) {
            return CoinType.FIL;
        } else if (txDataUnion.which() == TxDataUnion.Tag.SolanaTxData) {
            return CoinType.SOL;
        } else {
            return CoinType.ETH;
        }
    }

    public static @StringRes int getTxType(TransactionInfo info) {
        if (info == null) return R.string.wallet_details_function_type_other;
        switch (info.txType) {
            case TransactionType.ERC20_TRANSFER:
                return R.string.wallet_details_function_type_erc20transfer;
            case TransactionType.ERC20_APPROVE:
                return R.string.wallet_details_function_type_erc20approve;
            case TransactionType.ERC721_TRANSFER_FROM:
                return R.string.wallet_details_function_type_erc721transfer;
            case TransactionType.SOLANA_SYSTEM_TRANSFER:
                return R.string.wallet_details_function_type_solana_system_transfer;
            case TransactionType.SOLANA_SPL_TOKEN_TRANSFER:
                return R.string.wallet_details_function_type_spl_token_transfer;
            case TransactionType.SOLANA_SPL_TOKEN_TRANSFER_WITH_ASSOCIATED_TOKEN_ACCOUNT_CREATION:
                return R.string
                        .wallet_details_function_type_solana_spl_token_transfer_with_associated_token_account_creation;
            case TransactionType.SOLANA_DAPP_SIGN_AND_SEND_TRANSACTION:
                return R.string.wallet_details_function_type_solana_dapp_sign_and_send;
            case TransactionType.SOLANA_DAPP_SIGN_TRANSACTION:
                return R.string.wallet_details_function_type_solana_dapp_sign;
            default:
                return R.string.wallet_details_function_type_other;
        }
    }

    // ---------- Solana ----------
    public static boolean isSolanaTx(TransactionInfo transactionInfo) {
        if (transactionInfo == null || transactionInfo.txDataUnion == null) return false;
        return WalletConstants.SOLANA_TRANSACTION_TYPES.contains(transactionInfo.txType)
                || transactionInfo.txType == TransactionType.OTHER
                        && safeSolData(transactionInfo.txDataUnion) != null;
    }

    public static String getSolanaProgramIdName(String programId, Context context) {
        if (TextUtils.isEmpty(programId)) return "";
        switch (programId) {
            case WalletConstants.SOL_INS_SYSTEM:
                return context.getString(R.string.brave_wallet_solana_system_program);
            case WalletConstants.SOL_INS_CONFIG:
                return context.getString(R.string.brave_wallet_solana_config_program);
            case WalletConstants.SOL_INS_STAKE:
                return context.getString(R.string.brave_wallet_solana_stake_program);
            case WalletConstants.SOL_INS_VOTE:
                return context.getString(R.string.brave_wallet_solana_vote_program);
            case WalletConstants.SOL_INS_BPF:
                return context.getString(R.string.brave_wallet_solana_bp_floader);
            case WalletConstants.SOL_INS_SIG_VERIFY:
                return context.getString(R.string.brave_wallet_solana_ed25519_program);
            case WalletConstants.SOL_INS_TOKEN:
                return context.getString(R.string.brave_wallet_solana_token_program);
            case WalletConstants.SOL_INS_SECP:
                return context.getString(R.string.brave_wallet_solana_secp256k1_program);
            default:
                return programId;
        }
    }

    public static int getSolTxSubType(String programId, int instructionType) {
        if (TextUtils.isEmpty(programId)) return R.string.brave_wallet_unknown;

        switch (programId) {
            case WalletConstants.SOL_INS_SYSTEM:
                return getSolInsSysName(instructionType);
            case WalletConstants.SOL_INS_TOKEN:
                return getSolInsTokenName(instructionType);
            default:
                return R.string.brave_wallet_unknown;
        }
    }

    public static boolean isSolTxUnknown(String programId) {
        if (TextUtils.isEmpty(programId)) return true;
        switch (programId) {
            case WalletConstants.SOL_INS_SYSTEM:
            case WalletConstants.SOL_INS_VOTE:
            case WalletConstants.SOL_INS_STAKE:
            case WalletConstants.SOL_INS_TOKEN:
                return false;
            default:
                return true;
        }
    }

    public static List<SolanaTxData> safeSolData(SignSolTransactionsRequest request) {
        if (request == null || request.txDatas == null) return Collections.emptyList();
        List<SolanaTxData> txDatas = new ArrayList<>();
        for (SolanaTxData txData : request.txDatas) {
            if (txData != null) {
                txDatas.add(txData);
            }
        }
        return txDatas;
    }

    public static SolanaTxData safeSolData(TxDataUnion txDataUnion) {
        if (txDataUnion != null && txDataUnion.which() == TxDataUnion.Tag.SolanaTxData) {
            return txDataUnion.getSolanaTxData();
        }
        return null;
    }

    public static @StringRes int getSolInsSysName(int type) {
        switch (type) {
            case SolanaSystemInstruction.CREATE_ACCOUNT:
                return R.string.brave_wallet_tx_create_account;
            case SolanaSystemInstruction.ASSIGN:
                return R.string.brave_wallet_tx_assign;
            case SolanaSystemInstruction.TRANSFER:
                return R.string.brave_wallet_tx_transfer;
            case SolanaSystemInstruction.CREATE_ACCOUNT_WITH_SEED:
                return R.string.brave_wallet_tx_create_account_with_seed;
            case SolanaSystemInstruction.ADVANCE_NONCE_ACCOUNT:
                return R.string.brave_wallet_tx_advance_nonce_account;
            case SolanaSystemInstruction.WITHDRAW_NONCE_ACCOUNT:
                return R.string.brave_wallet_tx_withdraw_nonce_account;
            case SolanaSystemInstruction.INITIALIZE_NONCE_ACCOUNT:
                return R.string.brave_wallet_tx_initialize_nonce_account;
            case SolanaSystemInstruction.AUTHORIZE_NONCE_ACCOUNT:
                return R.string.brave_wallet_tx_authorize_nonce_account;
            case SolanaSystemInstruction.ALLOCATE:
                return R.string.brave_wallet_tx_allocate;
            case SolanaSystemInstruction.ALLOCATE_WITH_SEED:
                return R.string.brave_wallet_tx_allocate_with_seed;
            case SolanaSystemInstruction.ASSIGN_WITH_SEED:
                return R.string.brave_wallet_tx_assign_with_seed;
            case SolanaSystemInstruction.TRANSFER_WITH_SEED:
                return R.string.brave_wallet_tx_transfer_with_seed;
            case SolanaSystemInstruction.UPGRADE_NONCE_ACCOUNT:
                return R.string.brave_wallet_tx_upgrade_nonce_account;
            default:
                return R.string.brave_wallet_unknown;
        }
    }

    public static @StringRes int getSolInsTokenName(int type) {
        switch (type) {
            case SolanaTokenInstruction.INITIALIZE_MINT:
                return R.string.brave_wallet_tx_initialize_mint;
            case SolanaTokenInstruction.INITIALIZE_ACCOUNT:
                return R.string.brave_wallet_tx_initialize_account;
            case SolanaTokenInstruction.INITIALIZE_MULTISIG:
                return R.string.brave_wallet_tx_initialize_multisig;
            case SolanaTokenInstruction.TRANSFER:
                return R.string.brave_wallet_tx_transfer;
            case SolanaTokenInstruction.APPROVE:
                return R.string.brave_wallet_tx_approve;
            case SolanaTokenInstruction.REVOKE:
                return R.string.brave_wallet_tx_revoke;
            case SolanaTokenInstruction.SET_AUTHORITY:
                return R.string.brave_wallet_tx_set_authority;
            case SolanaTokenInstruction.MINT_TO:
                return R.string.brave_wallet_tx_mint_to;
            case SolanaTokenInstruction.BURN:
                return R.string.brave_wallet_tx_burn;
            case SolanaTokenInstruction.CLOSE_ACCOUNT:
                return R.string.brave_wallet_tx_close_account;
            case SolanaTokenInstruction.FREEZE_ACCOUNT:
                return R.string.brave_wallet_tx_freeze_account;
            case SolanaTokenInstruction.THAW_ACCOUNT:
                return R.string.brave_wallet_tx_thaw_account;
            case SolanaTokenInstruction.TRANSFER_CHECKED:
                return R.string.brave_wallet_tx_transfer_checked;
            case SolanaTokenInstruction.APPROVE_CHECKED:
                return R.string.brave_wallet_tx_approve_checked;
            case SolanaTokenInstruction.MINT_TO_CHECKED:
                return R.string.brave_wallet_tx_mint_to_checked;
            case SolanaTokenInstruction.BURN_CHECKED:
                return R.string.brave_wallet_tx_burn_checked;
            case SolanaTokenInstruction.INITIALIZE_ACCOUNT2:
                return R.string.brave_wallet_tx_initialize_account2;
            case SolanaTokenInstruction.SYNC_NATIVE:
                return R.string.brave_wallet_tx_sync_native;
            case SolanaTokenInstruction.INITIALIZE_ACCOUNT3:
                return R.string.brave_wallet_tx_initialize_account3;
            case SolanaTokenInstruction.INITIALIZE_MULTISIG2:
                return R.string.brave_wallet_tx_initialize_multisig2;
            case SolanaTokenInstruction.INITIALIZE_MINT2:
                return R.string.brave_wallet_tx_initialize_mint2;
            default:
                return R.string.brave_wallet_unknown;
        }
    }
    // ---------- Solana ----------
}
