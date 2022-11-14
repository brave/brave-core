/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_data_decoder.h"

#include <tuple>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/sys_byteorder.h"
#include "brave/components/brave_wallet/browser/solana_data_decoder_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "build/build_config.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet::solana_ins_data_decoder {

namespace {

enum class ParamTypes {
  kUint8,
  kUint32,
  kUint64,
  kPublicKey,
  kOptionalPublicKey,
  kString,
  kAuthorityType,
};

// Tuple of param name, localized name, and type.
using ParamNameTypeTuple = std::tuple<std::string, std::string, ParamTypes>;

const base::flat_map<mojom::SolanaSystemInstruction,
                     std::vector<ParamNameTypeTuple>>&
GetSystemInstructionParams() {
  static base::NoDestructor<base::flat_map<mojom::SolanaSystemInstruction,
                                           std::vector<ParamNameTypeTuple>>>
      params(
          {{mojom::SolanaSystemInstruction::kCreateAccount,
            {{"lamports",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_LAMPORTS),
              ParamTypes::kUint64},
             {"space",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SPACE),
              ParamTypes::kUint64},
             {"owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER_PROGRAM),
              ParamTypes::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kAssign,
            {{"owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER_PROGRAM),
              ParamTypes::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kTransfer,
            {{"lamports",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_LAMPORTS),
              ParamTypes::kUint64}}},
           {mojom::SolanaSystemInstruction::kCreateAccountWithSeed,
            {{"base",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_BASE),
              ParamTypes::kPublicKey},
             {"seed",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SEED),
              ParamTypes::kString},
             {"lamports",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_LAMPORTS),
              ParamTypes::kUint64},
             {"space",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SPACE),
              ParamTypes::kUint64},
             {"owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER_PROGRAM),
              ParamTypes::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kAdvanceNonceAccount, {}},
           {mojom::SolanaSystemInstruction::kWithdrawNonceAccount,
            {{"lamports",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_LAMPORTS),
              ParamTypes::kUint64}}},
           {mojom::SolanaSystemInstruction::kInitializeNonceAccount,
            {{"authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AUTHORITY),
              ParamTypes::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kAuthorizeNonceAccount,
            {{"new_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_NEW_AUTHORITY),
              ParamTypes::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kAllocate,
            {{"space",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SPACE),
              ParamTypes::kUint64}}},
           {mojom::SolanaSystemInstruction::kAllocateWithSeed,
            {{"base",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_BASE),
              ParamTypes::kPublicKey},
             {"seed",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SEED),
              ParamTypes::kString},
             {"space",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SPACE),
              ParamTypes::kUint64},
             {"owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER_PROGRAM),
              ParamTypes::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kAssignWithSeed,
            {{"base",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_BASE),
              ParamTypes::kPublicKey},
             {"seed",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SEED),
              ParamTypes::kString},
             {"owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER_PROGRAM),
              ParamTypes::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kTransferWithSeed,
            {{"lamports",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_LAMPORTS),
              ParamTypes::kUint64},
             {"from_seed",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_FROM_SEED),
              ParamTypes::kString},
             {"from_owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_FROM_OWNER_PROGRAM),
              ParamTypes::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kUpgradeNonceAccount, {}}});
  DCHECK(params->size() ==
         static_cast<uint32_t>(mojom::SolanaSystemInstruction::kMaxValue) + 1);
  return *params;
}

const base::flat_map<mojom::SolanaTokenInstruction,
                     std::vector<ParamNameTypeTuple>>&
GetTokenInstructionParams() {
  static base::NoDestructor<base::flat_map<mojom::SolanaTokenInstruction,
                                           std::vector<ParamNameTypeTuple>>>
      params(
          {{mojom::SolanaTokenInstruction::kInitializeMint,
            {{"decimals",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              ParamTypes::kUint8},
             {"mint_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_MINT_AUTHORITY),
              ParamTypes::kPublicKey},
             {"freeze_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_FREEZE_AUTHORITY),
              ParamTypes::kOptionalPublicKey}}},
           {mojom::SolanaTokenInstruction::kInitializeAccount, {}},
           {mojom::SolanaTokenInstruction::kInitializeMultisig,
            {{"num_of_signers",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_NUM_OF_SIGNERS),
              ParamTypes::kUint8}}},
           {mojom::SolanaTokenInstruction::kTransfer,
            {{"amount",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              ParamTypes::kUint64}}},
           {mojom::SolanaTokenInstruction::kApprove,
            {{"amount",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              ParamTypes::kUint64}}},
           {mojom::SolanaTokenInstruction::kRevoke, {}},
           {mojom::SolanaTokenInstruction::kSetAuthority,
            {{"authority_type",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AUTHORITY_TYPE),
              ParamTypes::kAuthorityType},
             {"new_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_NEW_AUTHORITY),
              ParamTypes::kOptionalPublicKey}}},
           {mojom::SolanaTokenInstruction::kMintTo,
            {{"amount",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              ParamTypes::kUint64}}},
           {mojom::SolanaTokenInstruction::kBurn,
            {{"amount",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              ParamTypes::kUint64}}},
           {mojom::SolanaTokenInstruction::kCloseAccount, {}},
           {mojom::SolanaTokenInstruction::kFreezeAccount, {}},
           {mojom::SolanaTokenInstruction::kThawAccount, {}},
           {mojom::SolanaTokenInstruction::kTransferChecked,
            {{"amount",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              ParamTypes::kUint64},
             {"decimals",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              ParamTypes::kUint8}}},
           {mojom::SolanaTokenInstruction::kApproveChecked,
            {{"amount",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              ParamTypes::kUint64},
             {"decimals",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              ParamTypes::kUint8}}},
           {mojom::SolanaTokenInstruction::kMintToChecked,
            {{"amount",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              ParamTypes::kUint64},
             {"decimals",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              ParamTypes::kUint8}}},
           {mojom::SolanaTokenInstruction::kBurnChecked,
            {{"amount",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              ParamTypes::kUint64},
             {"decimals",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              ParamTypes::kUint8}}},
           {mojom::SolanaTokenInstruction::kInitializeAccount2,
            {{"owner",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER),
              ParamTypes::kPublicKey}}},
           {mojom::SolanaTokenInstruction::kSyncNative, {}},
           {mojom::SolanaTokenInstruction::kInitializeAccount3,
            {{"owner",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER),
              ParamTypes::kPublicKey}}},
           {mojom::SolanaTokenInstruction::kInitializeMultisig2,
            {{"num_of_signers",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_NUM_OF_SIGNERS),
              ParamTypes::kUint8}}},
           {mojom::SolanaTokenInstruction::kInitializeMint2,
            {{"decimals",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              ParamTypes::kUint8},
             {"mint_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_MINT_AUTHORITY),
              ParamTypes::kPublicKey},
             {"freeze_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_FREEZE_AUTHORITY),
              ParamTypes::kOptionalPublicKey}}}});
  DCHECK(params->size() ==
         static_cast<uint32_t>(mojom::SolanaTokenInstruction::kMaxValue) + 1);
  return *params;
}

const base::flat_map<mojom::SolanaSystemInstruction, std::vector<InsParamPair>>&
GetSystemInstructionAccountParams() {
  static base::NoDestructor<
      base::flat_map<mojom::SolanaSystemInstruction, std::vector<InsParamPair>>>
      params(
          {{mojom::SolanaSystemInstruction::kCreateAccount,
            {{"from_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {"new_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NEW_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kAssign,
            {{"assigned_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ASSIGNED_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kTransfer,
            {{"from_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {"to_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kCreateAccountWithSeed,
            {{"from_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {"created_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_CREATED_ACCOUNT)},
             {"base_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_BASE_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kAdvanceNonceAccount,
            {{"nonce_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_ACCOUNT)},
             {"recentblockhashes_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RECENTBLOCKHASHES_SYSVAR)},  // NOLINT
             {"nonce_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_AUTHORITY)}}},
           {mojom::SolanaSystemInstruction::kWithdrawNonceAccount,
            {{"nonce_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_ACCOUNT)},
             {"to_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)},
             {"recentblockhashes_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RECENTBLOCKHASHES_SYSVAR)},  // NOLINT
             {"rent_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RENT_SYSVAR)},
             {"nonce_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_AUTHORITY)}}},
           {mojom::SolanaSystemInstruction::kInitializeNonceAccount,
            {{"nonce_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_ACCOUNT)},
             {"recentblockhashes_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RECENTBLOCKHASHES_SYSVAR)},  // NOLINT
             {"rent_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RENT_SYSVAR)}}},
           {mojom::SolanaSystemInstruction::kAuthorizeNonceAccount,
            {{"nonce_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_ACCOUNT)},
             {"nonce_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_AUTHORITY)}}},
           {mojom::SolanaSystemInstruction::kAllocate,
            {{"allocated_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ALLOCATED_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kAllocateWithSeed,
            {{"allocated_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ALLOCATED_ACCOUNT)},
             {"base_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_BASE_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kAssignWithSeed,
            {{"assigned_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ASSIGNED_ACCOUNT)},
             {"base_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_BASE_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kTransferWithSeed,
            {{"from_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {"base_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_BASE_ACCOUNT)},
             {"to_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kUpgradeNonceAccount,
            {{"nonce_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_ACCOUNT)}}}});
  DCHECK(params->size() ==
         static_cast<uint32_t>(mojom::SolanaSystemInstruction::kMaxValue) + 1);
  return *params;
}

const base::flat_map<mojom::SolanaTokenInstruction, std::vector<InsParamPair>>&
GetTokenInstructionAccountParams() {
  static base::NoDestructor<
      base::flat_map<mojom::SolanaTokenInstruction, std::vector<InsParamPair>>>
      params(
          {{mojom::SolanaTokenInstruction::kInitializeMint,
            {{"initialized_mint",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_INITIALIZED_MINT)},
             {"rent_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RENT_SYSVAR)}}},
           {mojom::SolanaTokenInstruction::kInitializeAccount,
            {{"initialized_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_INITIALIZED_ACCOUNT)},
             {"mint", l10n_util::GetStringUTF8(
                          IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_MINT)},
             {"owner", l10n_util::GetStringUTF8(
                           IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER)},
             {"rent_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RENT_SYSVAR)}}},
           {mojom::SolanaTokenInstruction::kInitializeMultisig,
            {{"initialized_multisig_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_INITIALIZED_MULTISIG_ACCOUNT)},  // NOLINT
             {"rent_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RENT_SYSVAR)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kTransfer,
            {{"from_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {"to_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)},
             {"owner_delegate",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER_DELEGATE)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kApprove,
            {{"account", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ACCOUNT)},
             {"delegate", l10n_util::GetStringUTF8(
                              IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_DELEGATE)},
             {"owner", l10n_util::GetStringUTF8(
                           IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kRevoke,
            {{"account", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ACCOUNT)},
             {"owner", l10n_util::GetStringUTF8(
                           IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kSetAuthority,
            {{"changed_mint_or_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_CHANGED_MINT_OR_ACCOUNT)},  // NOLINT
             {"authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_AUTHORITY)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kMintTo,
            {{"mint", l10n_util::GetStringUTF8(
                          IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_MINT)},
             {"to_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)},
             {"authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_AUTHORITY)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kBurn,
            {{"burned_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_BURNED_ACCOUNT)},
             {"token_mint",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TOKEN_MINT)},
             {"owner_delegate",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ACCOUNT_OWNER_DELEGATE)},  // NOLINT
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kCloseAccount,
            {{"closed_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_CLOSED_ACCOUNT)},
             {"to_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)},
             {"owner", l10n_util::GetStringUTF8(
                           IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kFreezeAccount,
            {{"frozen_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROZEN_ACCOUNT)},
             {"token_mint",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TOKEN_MINT)},
             {"authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_AUTHORITY)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kThawAccount,
            {{"frozen_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROZEN_ACCOUNT)},
             {"token_mint",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TOKEN_MINT)},
             {"authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_AUTHORITY)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kTransferChecked,
            {{"from_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {"token_mint",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TOKEN_MINT)},
             {"to_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)},
             {"owner_delegate",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER_DELEGATE)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kApproveChecked,
            {{"account", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ACCOUNT)},
             {"token_mint",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TOKEN_MINT)},
             {"delegate", l10n_util::GetStringUTF8(
                              IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_DELEGATE)},
             {"owner", l10n_util::GetStringUTF8(
                           IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kMintToChecked,
            {{"mint", l10n_util::GetStringUTF8(
                          IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_MINT)},
             {"to_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)},
             {"authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_AUTHORITY)},
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kBurnChecked,
            {{"burned_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_BURNED_ACCOUNT)},
             {"token_mint",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TOKEN_MINT)},
             {"owner_delegate",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ACCOUNT_OWNER_DELEGATE)},  // NOLINT
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kInitializeAccount2,
            {{"initialized_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_INITIALIZED_ACCOUNT)},
             {"mint", l10n_util::GetStringUTF8(
                          IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_MINT)},
             {"rent_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RENT_SYSVAR)}}},
           {mojom::SolanaTokenInstruction::kSyncNative,
            {{"native_token_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NATIVE_TOKEN_ACCOUNT)}}},  // NOLINT
           {mojom::SolanaTokenInstruction::kInitializeAccount3,
            {{"initialized_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_INITIALIZED_ACCOUNT)},
             {"mint", l10n_util::GetStringUTF8(
                          IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_MINT)}}},
           {mojom::SolanaTokenInstruction::kInitializeMultisig2,
            {{"initialized_multisig_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_INITIALIZED_MULTISIG_ACCOUNT)},  // NOLINT
             {"signers", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kInitializeMint2,
            {{"initialized_mint",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_INITIALIZED_MINT)}}}});
  DCHECK(params->size() ==
         static_cast<uint32_t>(mojom::SolanaTokenInstruction::kMaxValue) + 1);
  return *params;
}

absl::optional<mojom::SolanaSystemInstruction> DecodeSystemInstructionType(
    const std::vector<uint8_t>& data,
    size_t& offset) {
  auto ins_type = DecodeUint32(data, offset);
  if (!ins_type || *ins_type > static_cast<uint32_t>(
                                   mojom::SolanaSystemInstruction::kMaxValue))
    return absl::nullopt;
  return static_cast<mojom::SolanaSystemInstruction>(*ins_type);
}

absl::optional<mojom::SolanaTokenInstruction> DecodeTokenInstructionType(
    const std::vector<uint8_t>& data,
    size_t& offset) {
  auto ins_type = DecodeUint8(data, offset);
  if (!ins_type || *ins_type > static_cast<uint8_t>(
                                   mojom::SolanaTokenInstruction::kMaxValue))
    return absl::nullopt;
  return static_cast<mojom::SolanaTokenInstruction>(*ins_type);
}

const std::vector<ParamNameTypeTuple>* DecodeInstructionType(
    const std::string& program_id,
    const std::vector<uint8_t>& data,
    size_t& offset,
    SolanaInstructionDecodedData& decoded_data) {
  if (program_id == mojom::kSolanaSystemProgramId) {
    if (auto ins_type = DecodeSystemInstructionType(data, offset)) {
      auto* ret = &GetSystemInstructionParams().at(*ins_type);
      decoded_data.sys_ins_type = std::move(ins_type);
      decoded_data.account_params =
          GetSystemInstructionAccountParams().at(*ins_type);
      return ret;
    }
  } else if (program_id == mojom::kSolanaTokenProgramId) {
    if (auto ins_type = DecodeTokenInstructionType(data, offset)) {
      auto* ret = &GetTokenInstructionParams().at(*ins_type);
      decoded_data.token_ins_type = std::move(ins_type);
      decoded_data.account_params =
          GetTokenInstructionAccountParams().at(*ins_type);
      return ret;
    }
  }

  return nullptr;
}

}  // namespace

bool DecodeParamType(const ParamNameTypeTuple& name_type_tuple,
                     const std::vector<std::uint8_t> data,
                     size_t& offset,
                     std::vector<InsParamTuple>& ins_param_tuple) {
  absl::optional<std::string> value;

  switch (std::get<2>(name_type_tuple)) {
    case ParamTypes::kUint8:
      value = DecodeUint8String(data, offset);
      break;
    case ParamTypes::kUint32:
      value = DecodeUint32String(data, offset);
      break;
    case ParamTypes::kUint64:
      value = DecodeUint64String(data, offset);
      break;
    case ParamTypes::kPublicKey:
      value = DecodePublicKey(data, offset);
      break;
    case ParamTypes::kOptionalPublicKey:
      value = DecodeOptionalPublicKey(data, offset);
      break;
    case ParamTypes::kString:
      value = DecodeString(data, offset);
      break;
    case ParamTypes::kAuthorityType:
      value = DecodeAuthorityTypeString(data, offset);
  }

  if (!value)
    return false;

  // Early return to ignore optional public key in the param name-value list
  // if it is not passed.
  if (std::get<2>(name_type_tuple) == ParamTypes::kOptionalPublicKey &&
      value->empty()) {
    return true;
  }

  ins_param_tuple.emplace_back(std::get<0>(name_type_tuple),
                               std::get<1>(name_type_tuple), *value);
  return true;
}

absl::optional<SolanaInstructionDecodedData> Decode(
    const std::vector<uint8_t>& data,
    const std::string& program_id) {
  if (program_id != mojom::kSolanaSystemProgramId &&
      program_id != mojom::kSolanaTokenProgramId) {
    return absl::nullopt;
  }

  SolanaInstructionDecodedData decoded_data;
  size_t offset = 0;

  const std::vector<ParamNameTypeTuple>* param_tuples =
      DecodeInstructionType(program_id, data, offset, decoded_data);
  if (!param_tuples)
    return absl::nullopt;

  for (const auto& param_tuple : *param_tuples) {
    if (!DecodeParamType(param_tuple, data, offset, decoded_data.params)) {
      return absl::nullopt;
    }
  }

  return decoded_data;
}

std::vector<InsParamPair> GetAccountParamsForTesting(
    absl::optional<mojom::SolanaSystemInstruction> sys_ins_type,
    absl::optional<mojom::SolanaTokenInstruction> token_ins_type) {
  if (sys_ins_type) {
    return GetSystemInstructionAccountParams().at(*sys_ins_type);
  }

  if (token_ins_type) {
    return GetTokenInstructionAccountParams().at(*token_ins_type);
  }

  NOTREACHED();
  return std::vector<InsParamPair>();
}

std::vector<mojom::SolanaInstructionAccountParamPtr>
GetMojomAccountParamsForTesting(
    absl::optional<mojom::SolanaSystemInstruction> sys_ins_type,
    absl::optional<mojom::SolanaTokenInstruction> token_ins_type) {
  std::vector<mojom::SolanaInstructionAccountParamPtr> mojom_params;
  for (const auto& param :
       GetAccountParamsForTesting(sys_ins_type, token_ins_type)) {
    mojom_params.push_back(
        mojom::SolanaInstructionAccountParam::New(param.first, param.second));
  }
  return mojom_params;
}

}  // namespace brave_wallet::solana_ins_data_decoder
