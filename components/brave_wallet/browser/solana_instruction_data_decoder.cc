/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_instruction_data_decoder.h"

#include <optional>
#include <tuple>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/numerics/byte_conversions.h"
#include "base/sys_byteorder.h"
#include "brave/components/brave_wallet/browser/solana_instruction_builder.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "build/build_config.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet::solana_ins_data_decoder {

namespace {

constexpr uint8_t kAuthorityTypeMax = 3;
constexpr size_t kMaxStringSize32Bit = 4294967291u;

// Tuple of param name, localized name, and type.
using ParamNameTypeTuple =
    std::tuple<std::string, std::string, mojom::SolanaInstructionParamType>;

const base::flat_map<mojom::SolanaSystemInstruction,
                     std::vector<ParamNameTypeTuple>>&
GetSystemInstructionParams() {
  static base::NoDestructor<base::flat_map<mojom::SolanaSystemInstruction,
                                           std::vector<ParamNameTypeTuple>>>
      params(
          {{mojom::SolanaSystemInstruction::kCreateAccount,
            {{mojom::kLamports,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_LAMPORTS),
              mojom::SolanaInstructionParamType::kUint64},
             {"space",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SPACE),
              mojom::SolanaInstructionParamType::kUint64},
             {"owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER_PROGRAM),
              mojom::SolanaInstructionParamType::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kAssign,
            {{"owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER_PROGRAM),
              mojom::SolanaInstructionParamType::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kTransfer,
            {{mojom::kLamports,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_LAMPORTS),
              mojom::SolanaInstructionParamType::kUint64}}},
           {mojom::SolanaSystemInstruction::kCreateAccountWithSeed,
            {{"base",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_BASE),
              mojom::SolanaInstructionParamType::kPublicKey},
             {"seed",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SEED),
              mojom::SolanaInstructionParamType::kString},
             {mojom::kLamports,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_LAMPORTS),
              mojom::SolanaInstructionParamType::kUint64},
             {"space",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SPACE),
              mojom::SolanaInstructionParamType::kUint64},
             {"owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER_PROGRAM),
              mojom::SolanaInstructionParamType::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kAdvanceNonceAccount, {}},
           {mojom::SolanaSystemInstruction::kWithdrawNonceAccount,
            {{mojom::kLamports,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_LAMPORTS),
              mojom::SolanaInstructionParamType::kUint64}}},
           {mojom::SolanaSystemInstruction::kInitializeNonceAccount,
            {{"authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AUTHORITY),
              mojom::SolanaInstructionParamType::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kAuthorizeNonceAccount,
            {{"new_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_NEW_AUTHORITY),
              mojom::SolanaInstructionParamType::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kAllocate,
            {{"space",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SPACE),
              mojom::SolanaInstructionParamType::kUint64}}},
           {mojom::SolanaSystemInstruction::kAllocateWithSeed,
            {{"base",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_BASE),
              mojom::SolanaInstructionParamType::kPublicKey},
             {"seed",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SEED),
              mojom::SolanaInstructionParamType::kString},
             {"space",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SPACE),
              mojom::SolanaInstructionParamType::kUint64},
             {"owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER_PROGRAM),
              mojom::SolanaInstructionParamType::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kAssignWithSeed,
            {{"base",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_BASE),
              mojom::SolanaInstructionParamType::kPublicKey},
             {"seed",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_SEED),
              mojom::SolanaInstructionParamType::kString},
             {"owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER_PROGRAM),
              mojom::SolanaInstructionParamType::kPublicKey}}},
           {mojom::SolanaSystemInstruction::kTransferWithSeed,
            {{mojom::kLamports,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_LAMPORTS),
              mojom::SolanaInstructionParamType::kUint64},
             {"from_seed",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_FROM_SEED),
              mojom::SolanaInstructionParamType::kString},
             {"from_owner_program",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_FROM_OWNER_PROGRAM),
              mojom::SolanaInstructionParamType::kPublicKey}}},
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
            {{mojom::kDecimals,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              mojom::SolanaInstructionParamType::kUint8},
             {"mint_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_MINT_AUTHORITY),
              mojom::SolanaInstructionParamType::kPublicKey},
             {"freeze_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_FREEZE_AUTHORITY),
              mojom::SolanaInstructionParamType::kOptionalPublicKey}}},
           {mojom::SolanaTokenInstruction::kInitializeAccount, {}},
           {mojom::SolanaTokenInstruction::kInitializeMultisig,
            {{"num_of_signers",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_NUM_OF_SIGNERS),
              mojom::SolanaInstructionParamType::kUint8}}},
           {mojom::SolanaTokenInstruction::kTransfer,
            {{mojom::kAmount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              mojom::SolanaInstructionParamType::kUint64}}},
           {mojom::SolanaTokenInstruction::kApprove,
            {{mojom::kAmount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              mojom::SolanaInstructionParamType::kUint64}}},
           {mojom::SolanaTokenInstruction::kRevoke, {}},
           {mojom::SolanaTokenInstruction::kSetAuthority,
            {{"authority_type",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AUTHORITY_TYPE),
              mojom::SolanaInstructionParamType::kAuthorityType},
             {"new_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_NEW_AUTHORITY),
              mojom::SolanaInstructionParamType::kOptionalPublicKey}}},
           {mojom::SolanaTokenInstruction::kMintTo,
            {{mojom::kAmount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              mojom::SolanaInstructionParamType::kUint64}}},
           {mojom::SolanaTokenInstruction::kBurn,
            {{mojom::kAmount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              mojom::SolanaInstructionParamType::kUint64}}},
           {mojom::SolanaTokenInstruction::kCloseAccount, {}},
           {mojom::SolanaTokenInstruction::kFreezeAccount, {}},
           {mojom::SolanaTokenInstruction::kThawAccount, {}},
           {mojom::SolanaTokenInstruction::kTransferChecked,
            {{mojom::kAmount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              mojom::SolanaInstructionParamType::kUint64},
             {mojom::kDecimals,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              mojom::SolanaInstructionParamType::kUint8}}},
           {mojom::SolanaTokenInstruction::kApproveChecked,
            {{mojom::kAmount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              mojom::SolanaInstructionParamType::kUint64},
             {mojom::kDecimals,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              mojom::SolanaInstructionParamType::kUint8}}},
           {mojom::SolanaTokenInstruction::kMintToChecked,
            {{mojom::kAmount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              mojom::SolanaInstructionParamType::kUint64},
             {mojom::kDecimals,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              mojom::SolanaInstructionParamType::kUint8}}},
           {mojom::SolanaTokenInstruction::kBurnChecked,
            {{mojom::kAmount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_AMOUNT),
              mojom::SolanaInstructionParamType::kUint64},
             {mojom::kDecimals,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              mojom::SolanaInstructionParamType::kUint8}}},
           {mojom::SolanaTokenInstruction::kInitializeAccount2,
            {{"owner",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER),
              mojom::SolanaInstructionParamType::kPublicKey}}},
           {mojom::SolanaTokenInstruction::kSyncNative, {}},
           {mojom::SolanaTokenInstruction::kInitializeAccount3,
            {{"owner",
              l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SOLANA_INS_PARAM_OWNER),
              mojom::SolanaInstructionParamType::kPublicKey}}},
           {mojom::SolanaTokenInstruction::kInitializeMultisig2,
            {{"num_of_signers",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_NUM_OF_SIGNERS),
              mojom::SolanaInstructionParamType::kUint8}}},
           {mojom::SolanaTokenInstruction::kInitializeMint2,
            {{mojom::kDecimals,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_DECIMALS),
              mojom::SolanaInstructionParamType::kUint8},
             {"mint_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_MINT_AUTHORITY),
              mojom::SolanaInstructionParamType::kPublicKey},
             {"freeze_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_INS_PARAM_FREEZE_AUTHORITY),
              mojom::SolanaInstructionParamType::kOptionalPublicKey}}}});
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
            {{mojom::kFromAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {mojom::kNewAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NEW_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kAssign,
            {{"assigned_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ASSIGNED_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kTransfer,
            {{mojom::kFromAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {mojom::kToAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kCreateAccountWithSeed,
            {{mojom::kFromAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {"created_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_CREATED_ACCOUNT)},
             {"base_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_BASE_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kAdvanceNonceAccount,
            {{mojom::kNonceAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_ACCOUNT)},
             {"recentblockhashes_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RECENTBLOCKHASHES_SYSVAR)},  // NOLINT
             {"nonce_authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_AUTHORITY)}}},
           {mojom::SolanaSystemInstruction::kWithdrawNonceAccount,
            {{mojom::kNonceAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_ACCOUNT)},
             {mojom::kToAccount,
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
            {{mojom::kNonceAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_NONCE_ACCOUNT)},
             {"recentblockhashes_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RECENTBLOCKHASHES_SYSVAR)},  // NOLINT
             {"rent_sysvar",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_RENT_SYSVAR)}}},
           {mojom::SolanaSystemInstruction::kAuthorizeNonceAccount,
            {{mojom::kNonceAccount,
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
            {{mojom::kFromAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {"base_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_BASE_ACCOUNT)},
             {mojom::kToAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)}}},
           {mojom::SolanaSystemInstruction::kUpgradeNonceAccount,
            {{mojom::kNonceAccount,
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
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kTransfer,
            {{mojom::kFromAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {mojom::kToAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)},
             {"owner_delegate",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER_DELEGATE)},
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kApprove,
            {{"account", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ACCOUNT)},
             {"delegate", l10n_util::GetStringUTF8(
                              IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_DELEGATE)},
             {"owner", l10n_util::GetStringUTF8(
                           IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER)},
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kRevoke,
            {{"account", l10n_util::GetStringUTF8(
                             IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_ACCOUNT)},
             {"owner", l10n_util::GetStringUTF8(
                           IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER)},
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kSetAuthority,
            {{"changed_mint_or_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_CHANGED_MINT_OR_ACCOUNT)},  // NOLINT
             {"authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_AUTHORITY)},
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kMintTo,
            {{"mint", l10n_util::GetStringUTF8(
                          IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_MINT)},
             {mojom::kToAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)},
             {"authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_AUTHORITY)},
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
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
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kCloseAccount,
            {{"closed_account",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_CLOSED_ACCOUNT)},
             {mojom::kToAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)},
             {"owner", l10n_util::GetStringUTF8(
                           IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER)},
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
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
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
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
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kTransferChecked,
            {{mojom::kFromAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_FROM_ACCOUNT)},
             {"token_mint",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TOKEN_MINT)},
             {mojom::kToAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)},
             {"owner_delegate",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_OWNER_DELEGATE)},
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
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
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kMintToChecked,
            {{"mint", l10n_util::GetStringUTF8(
                          IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_MINT)},
             {mojom::kToAccount,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_TO_ACCOUNT)},
             {"authority",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_AUTHORITY)},
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
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
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
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
             {mojom::kSigners,
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_SIGNERS)}}},
           {mojom::SolanaTokenInstruction::kInitializeMint2,
            {{"initialized_mint",
              l10n_util::GetStringUTF8(
                  IDS_BRAVE_WALLET_SOLANA_ACCOUNT_PARAM_INITIALIZED_MINT)}}}});
  DCHECK(params->size() ==
         static_cast<uint32_t>(mojom::SolanaTokenInstruction::kMaxValue) + 1);
  return *params;
}

std::optional<std::string> DecodeUint8String(base::span<const uint8_t> input,
                                             size_t& offset) {
  auto ret = DecodeUint8(input, offset);
  if (!ret) {
    return std::nullopt;
  }
  return base::NumberToString(*ret);
}

std::optional<std::string> DecodeAuthorityTypeString(
    base::span<const uint8_t> input,
    size_t& offset) {
  auto ret = DecodeUint8(input, offset);
  if (ret && *ret <= kAuthorityTypeMax) {
    return base::NumberToString(*ret);
  }
  return std::nullopt;
}

std::optional<std::string> DecodeUint32String(base::span<const uint8_t> input,
                                              size_t& offset) {
  auto ret = DecodeUint32(input, offset);
  if (!ret) {
    return std::nullopt;
  }
  return base::NumberToString(*ret);
}

std::optional<uint64_t> DecodeUint64(base::span<const uint8_t> input,
                                     size_t& offset) {
  if (offset >= input.size() || input.size() - offset < 8u) {
    return std::nullopt;
  }

  // Read bytes in little endian order.
  auto value = input.subspan(offset).first<8u>();
  offset += 8u;
  return base::U64FromLittleEndian(value);
}

std::optional<std::string> DecodeUint64String(base::span<const uint8_t> input,
                                              size_t& offset) {
  auto ret = DecodeUint64(input, offset);
  if (!ret) {
    return std::nullopt;
  }
  return base::NumberToString(*ret);
}

std::optional<std::string> DecodeOptionalPublicKey(
    base::span<const uint8_t> input,
    size_t& offset) {
  if (offset == input.size()) {
    return std::nullopt;
  }

  // First byte is 0 or 1 to indicate if public key is passed.
  // And the rest bytes are the actual public key.
  if (input[offset] == 0) {
    offset++;
    return "";  // No public key is passed.
  } else if (input[offset] == 1) {
    offset++;
    return DecodePublicKey(input, offset);
  } else {
    return std::nullopt;
  }
}

// bincode::serialize uses two u32 together for the string length and a byte
// array for the actual strings. The first u32 represents the lower bytes of
// the length, the second represents the upper bytes. The upper bytes will have
// non-zero value only when the length exceeds the maximum of u32.
// We currently cap the length here to be the max size of std::string
// on 32 bit systems, it's safe to do so because currently we don't expect any
// valid cases would have strings larger than it.
std::optional<std::string> DecodeString(base::span<const uint8_t> input,
                                        size_t& offset) {
  auto len_lower = DecodeUint32(input, offset);
  if (!len_lower || *len_lower > kMaxStringSize32Bit) {
    return std::nullopt;
  }
  auto len_upper = DecodeUint32(input, offset);
  if (!len_upper || *len_upper != 0) {  // Non-zero means len exceeds u32 max.
    return std::nullopt;
  }

  if (offset + *len_lower > input.size()) {
    return std::nullopt;
  }

  offset += *len_lower;
  return std::string(reinterpret_cast<const char*>(&input[offset - *len_lower]),
                     *len_lower);
}

bool DecodeParamType(const ParamNameTypeTuple& name_type_tuple,
                     base::span<const uint8_t> data,
                     size_t& offset,
                     std::vector<InsParamTuple>& ins_param_tuple) {
  std::optional<std::string> value;

  switch (std::get<2>(name_type_tuple)) {
    case mojom::SolanaInstructionParamType::kUint8:
      value = DecodeUint8String(data, offset);
      break;
    case mojom::SolanaInstructionParamType::kUint32:
      value = DecodeUint32String(data, offset);
      break;
    case mojom::SolanaInstructionParamType::kUint64:
      value = DecodeUint64String(data, offset);
      break;
    case mojom::SolanaInstructionParamType::kPublicKey:
      value = DecodePublicKey(data, offset);
      break;
    case mojom::SolanaInstructionParamType::kOptionalPublicKey:
      value = DecodeOptionalPublicKey(data, offset);
      break;
    case mojom::SolanaInstructionParamType::kString:
      value = DecodeString(data, offset);
      break;
    case mojom::SolanaInstructionParamType::kAuthorityType:
      value = DecodeAuthorityTypeString(data, offset);
      break;
    case mojom::SolanaInstructionParamType::kUnknown:
      // Do nothing
      break;
  }

  if (!value) {
    return false;
  }

  // Early return to ignore optional public key in the param name-value list
  // if it is not passed.
  if (std::get<2>(name_type_tuple) ==
          mojom::SolanaInstructionParamType::kOptionalPublicKey &&
      value->empty()) {
    return true;
  }

  ins_param_tuple.emplace_back(std::get<0>(name_type_tuple),
                               std::get<1>(name_type_tuple), *value,
                               std::get<2>(name_type_tuple));
  return true;
}

std::optional<mojom::SolanaSystemInstruction> DecodeSystemInstructionType(
    base::span<const uint8_t> data,
    size_t& offset) {
  auto ins_type = DecodeUint32(data, offset);
  if (!ins_type || *ins_type > static_cast<uint32_t>(
                                   mojom::SolanaSystemInstruction::kMaxValue)) {
    return std::nullopt;
  }
  return static_cast<mojom::SolanaSystemInstruction>(*ins_type);
}

std::optional<mojom::SolanaTokenInstruction> DecodeTokenInstructionType(
    base::span<const uint8_t> data,
    size_t& offset) {
  auto ins_type = DecodeUint8(data, offset);
  if (!ins_type || *ins_type > static_cast<uint8_t>(
                                   mojom::SolanaTokenInstruction::kMaxValue)) {
    return std::nullopt;
  }
  return static_cast<mojom::SolanaTokenInstruction>(*ins_type);
}

const std::vector<ParamNameTypeTuple>* DecodeInstructionType(
    const std::string& program_id,
    base::span<const uint8_t> data,
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
  } else if (program_id == mojom::kSolanaTokenProgramId ||
             program_id == mojom::kSolanaToken2022ProgramId) {
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

std::optional<SolanaInstructionDecodedData> Decode(
    base::span<const uint8_t> data,
    const std::string& program_id) {
  if (program_id != mojom::kSolanaSystemProgramId &&
      program_id != mojom::kSolanaTokenProgramId &&
      program_id != mojom::kSolanaToken2022ProgramId) {
    return std::nullopt;
  }

  SolanaInstructionDecodedData decoded_data;
  size_t offset = 0;

  const std::vector<ParamNameTypeTuple>* param_tuples =
      DecodeInstructionType(program_id, data, offset, decoded_data);
  if (!param_tuples) {
    return std::nullopt;
  }

  for (const auto& param_tuple : *param_tuples) {
    if (!DecodeParamType(param_tuple, data, offset, decoded_data.params)) {
      return std::nullopt;
    }
  }

  return decoded_data;
}

std::optional<uint8_t> DecodeUint8(base::span<const uint8_t> input,
                                   size_t& offset) {
  if (offset >= input.size() || input.size() - offset < sizeof(uint8_t)) {
    return std::nullopt;
  }

  auto result = input[offset];
  offset += sizeof(uint8_t);
  return result;
}

std::optional<uint32_t> DecodeUint32(base::span<const uint8_t> input,
                                     size_t& offset) {
  if (offset >= input.size() || input.size() - offset < sizeof(uint32_t)) {
    return std::nullopt;
  }

  // Read bytes in little endian order.
  base::span<const uint8_t> s =
      base::make_span(input.begin() + offset, sizeof(uint32_t));
  uint32_t uint32_le = *reinterpret_cast<const uint32_t*>(s.data());

  offset += sizeof(uint32_t);

  return uint32_le;
}

std::optional<std::string> DecodePublicKey(base::span<const uint8_t> input,
                                           size_t& offset) {
  if (offset >= input.size() || input.size() - offset < kSolanaPubkeySize) {
    return std::nullopt;
  }

  offset += kSolanaPubkeySize;
  return Base58Encode(std::vector<uint8_t>(
      input.begin() + offset - kSolanaPubkeySize, input.begin() + offset));
}

std::vector<InsParamPair> GetAccountParamsForTesting(
    std::optional<mojom::SolanaSystemInstruction> sys_ins_type,
    std::optional<mojom::SolanaTokenInstruction> token_ins_type) {
  if (sys_ins_type) {
    return GetSystemInstructionAccountParams().at(*sys_ins_type);
  }

  if (token_ins_type) {
    return GetTokenInstructionAccountParams().at(*token_ins_type);
  }

  NOTREACHED_IN_MIGRATION();
  return std::vector<InsParamPair>();
}

std::vector<mojom::SolanaInstructionAccountParamPtr>
GetMojomAccountParamsForTesting(
    std::optional<mojom::SolanaSystemInstruction> sys_ins_type,
    std::optional<mojom::SolanaTokenInstruction> token_ins_type) {
  std::vector<mojom::SolanaInstructionAccountParamPtr> mojom_params;
  for (const auto& param :
       GetAccountParamsForTesting(sys_ins_type, token_ins_type)) {
    mojom_params.push_back(
        mojom::SolanaInstructionAccountParam::New(param.first, param.second));
  }
  return mojom_params;
}

std::optional<mojom::SolanaSystemInstruction> GetSystemInstructionType(
    base::span<const uint8_t> data,
    const std::string& program_id) {
  if (program_id != mojom::kSolanaSystemProgramId) {
    return std::nullopt;
  }

  size_t offset = 0;
  return DecodeSystemInstructionType(data, offset);
}

std::optional<mojom::SolanaComputeBudgetInstruction>
GetComputeBudgetInstructionType(const std::vector<uint8_t>& data,
                                const std::string& program_id) {
  if (program_id != mojom::kSolanaComputeBudgetProgramId) {
    return std::nullopt;
  }

  if (data.empty()) {
    return std::nullopt;
  }

  uint8_t ins_type = data[0];  // First byte is the instruction type
  auto mojo_ins_type =
      static_cast<mojom::SolanaComputeBudgetInstruction>(ins_type);
  if (!mojom::IsKnownEnumValue(mojo_ins_type)) {
    return std::nullopt;
  }

  return mojo_ins_type;
}

bool IsCompressedNftTransferInstruction(const std::vector<uint8_t>& data,
                                        const std::string& program_id) {
  if (program_id != mojom::kSolanaBubbleGumProgramId) {
    return false;
  }

  if (data.size() <
      solana::bubblegum_program::kTransferInstructionDiscriminator.size()) {
    return false;
  }

  return std::equal(
      solana::bubblegum_program::kTransferInstructionDiscriminator.begin(),
      solana::bubblegum_program::kTransferInstructionDiscriminator.end(),
      data.begin());
}

}  // namespace brave_wallet::solana_ins_data_decoder
