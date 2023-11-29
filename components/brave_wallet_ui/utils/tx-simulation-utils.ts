// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet } from '../constants/types'

// constants
import { BLOWFISH_URL_WARNING_KINDS } from '../common/constants/blowfish'

// utils
import { getLocale } from '../../common/locale'

interface GroupedEVMStateChanges {
  evmApprovals: BraveWallet.BlowfishEVMStateChange[]
  evmTransfers: BraveWallet.BlowfishEVMStateChange[]
}

interface GroupedSVMStateChanges {
  solStakeAuthorityChanges: BraveWallet.BlowfishSolanaStateChange[]
  splApprovals: BraveWallet.BlowfishSolanaStateChange[]
  svmTransfers: BraveWallet.BlowfishSolanaStateChange[]
}

export const groupSimulatedEVMStateChanges = (
  evmStateChanges: BraveWallet.BlowfishEVMStateChange[]
): GroupedEVMStateChanges => {
  const changes: GroupedEVMStateChanges = {
    evmApprovals: [],
    evmTransfers: []
  }

  for (const stateChange of evmStateChanges) {
    const { data } = stateChange.rawInfo

    // approvals & approvals for all
    if (
      data.erc20ApprovalData ||
      data.erc721ApprovalData ||
      data.erc721ApprovalForAllData ||
      data.erc1155ApprovalForAllData
    ) {
      changes.evmApprovals.push(stateChange)
    }

    // transfers
    if (
      data.erc20TransferData ||
      data.erc721TransferData ||
      data.erc1155TransferData ||
      data.nativeAssetTransferData
    ) {
      changes.evmTransfers.push(stateChange)
    }
  }

  return changes
}

export const decodeSimulatedSVMStateChanges = (
  stateChanges: BraveWallet.BlowfishSolanaStateChange[]
): GroupedSVMStateChanges => {
  const changes: GroupedSVMStateChanges = {
    solStakeAuthorityChanges: [],
    splApprovals: [],
    svmTransfers: []
  }

  for (const stateChange of stateChanges) {
    const { data } = stateChange.rawInfo

    // staking auth changes
    if (data.solStakeAuthorityChangeData) {
      changes.solStakeAuthorityChanges.push(stateChange)
    }

    // approvals
    if (data.splApprovalData) {
      changes.splApprovals.push(stateChange)
    }

    // transfers
    if (data.solTransferData || data.splTransferData) {
      changes.svmTransfers.push(stateChange)
    }
  }

  return changes
}

export const isUrlWarning = (warningKind: BraveWallet.BlowfishWarningKind) => {
  return BLOWFISH_URL_WARNING_KINDS.includes(warningKind)
}

const { BlowfishEVMErrorKind, BlowfishSolanaErrorKind: SolErrorKind } =
  BraveWallet

export const translateSimulationWarning = (
  warning: BraveWallet.BlowfishWarning | undefined
) => {
  if (!warning) {
    return ''
  }

  switch (warning.kind) {
    case BraveWallet.BlowfishWarningKind.kApprovalToEOA:
      return getLocale('braveWalletSimulationWarningApprovalToEoa')

    case BraveWallet.BlowfishWarningKind.kBulkApprovalsRequest:
      return getLocale('braveWalletSimulationWarningBulkApprovalsRequest')

    case BraveWallet.BlowfishWarningKind.kCopyCatDomain:
    case BraveWallet.BlowfishWarningKind.kMultiCopyCatDomain:
      return getLocale('braveWalletSimulationWarningCopyCatDomain')

    case BraveWallet.BlowfishWarningKind.kDanglingApproval:
      return getLocale('braveWalletSimulationWarningDanglingApproval')

    case BraveWallet.BlowfishWarningKind.kKnownMalicious:
      return getLocale('braveWalletSimulationWarningKnownMalicious')

    case BraveWallet.BlowfishWarningKind.kNewDomain:
      return getLocale('braveWalletSimulationWarningNewDomain')

    case BraveWallet.BlowfishWarningKind.kPoisonedAddress:
      return getLocale('braveWalletSimulationWarningPoisonedAddress')

    case BraveWallet.BlowfishWarningKind.kSetOwnerAuthority:
      return getLocale('braveWalletSimulationWarningSetOwnerAuthority')

    case BraveWallet.BlowfishWarningKind.kSuspectedMalicious:
      return getLocale('braveWalletSimulationWarningSuspectedMalicious')

    case BraveWallet.BlowfishWarningKind.kTooManyTransactions:
      return warning.severity === BraveWallet.BlowfishWarningSeverity.kCritical
        ? getLocale('braveWalletSimulationWarningTooManyTransactionsCritical')
        : getLocale('braveWalletSimulationWarningTooManyTransactions')

    case BraveWallet.BlowfishWarningKind.kTradeForNothing:
      return getLocale('braveWalletSimulationWarningTradeForNothing')

    case BraveWallet.BlowfishWarningKind.kTransferringErc20ToOwnContract:
      return getLocale(
        'braveWalletSimulationWarningTransferringErc20ToOwnContract'
      )

    case BraveWallet.BlowfishWarningKind.kUserAccountOwnerChange:
      return getLocale('braveWalletSimulationWarningUserAccountOwnerChange')

    // Not translated
    case BraveWallet.BlowfishWarningKind.kBlocklistedDomainCrossOrigin:
    case BraveWallet.BlowfishWarningKind.kCompromisedAuthorityUpgrade:
    case BraveWallet.BlowfishWarningKind.kCopyCatImageUnresponsiveDomain:
    // case BraveWallet.BlowfishWarningKind.kDevtoolsDisabled:
    case BraveWallet.BlowfishWarningKind.kEthSignTxHash:
    // case BraveWallet.BlowfishWarningKind.kNonAsciiUrl:
    // case BraveWallet.BlowfishWarningKind.kObfuscatedCode:
    case BraveWallet.BlowfishWarningKind.kPermitNoExpiration:
    case BraveWallet.BlowfishWarningKind.kPermitUnlimitedAllowance:
    case BraveWallet.BlowfishWarningKind.kReferencedOfacAddress:
    case BraveWallet.BlowfishWarningKind.kSemiTrustedBlocklistDomain:
    case BraveWallet.BlowfishWarningKind.kTrustedBlocklistDomain:
    case BraveWallet.BlowfishWarningKind.kUnknown:
    case BraveWallet.BlowfishWarningKind.kUnlimitedAllowanceToNfts:
    case BraveWallet.BlowfishWarningKind.kUnusualGasConsumption:
    case BraveWallet.BlowfishWarningKind.kWhitelistedDomainCrossOrigin:

    default:
      return warning.message
  }
}

export const translateSimulationResultError = (
  error:
    | BraveWallet.BlowfishEVMError
    | BraveWallet.BlowfishSolanaError
    | undefined,
  /** prevents collisions between error enums */
  coinType: BraveWallet.CoinType
) => {
  if (!error) {
    return ''
  }

  // SVM
  if (coinType === BraveWallet.CoinType.SOL) {
    switch (error.kind) {
      case SolErrorKind.kAccountDoesNotHaveEnoughSolToPerformTheOperation: //
      case SolErrorKind.kInsufficientFunds:
        return getLocale('braveWalletSimulationErrorInsufficientFunds')

      case SolErrorKind.kInsufficientFundsForFee:
        return getLocale('braveWalletSimulationErrorInsufficientFundsForFee')

      case SolErrorKind.kTooManyTransactions:
        return getLocale(
          'braveWalletSimulationWarningTooManyTransactionsCritical'
        )

      // Not translated
      case SolErrorKind.kAccountDoesNotSupportSpecifiedAuthorityType:
      case SolErrorKind.kAccountInUse:
      case SolErrorKind.kAccountIsFrozen:
      case SolErrorKind.kAccountLoadedTwice:
      case SolErrorKind.kAccountNotAssociatedWithThisMint:
      // eslint-disable-next-line max-len
      case SolErrorKind.kAdvancingStoredNonceRequiresAPopulatedRecentblockhashesSysvar:
      case SolErrorKind.kAlreadyInUse:
      case SolErrorKind.kAnAccountWithTheSameAddressAlreadyExists:
      case SolErrorKind.kAttemptToDebitAnAccountButFoundNoRecordOfAPriorCredit:
      case SolErrorKind.kAttemptToLoadAProgramThatDoesNotExist:
      case SolErrorKind.kBadRequest:
      case SolErrorKind.kBlockhashNotFound:
      case SolErrorKind.kCannotAllocateAccountDataOfThisLength:
      case SolErrorKind.kCannotAssignAccountToThisProgramId:
      case SolErrorKind.kErrorProcessingInstruction:
      case SolErrorKind.kFixedSupply:
      case SolErrorKind.kInstructionDoesNotSupportNativeTokens:
      case SolErrorKind.kInvalidInstruction:
      case SolErrorKind.kInvalidMint:
      case SolErrorKind.kInvalidNumberOfProvidedSigners:
      case SolErrorKind.kInvalidNumberOfRequiredSigners:
      case SolErrorKind.kLamportBalanceBelowRentExemptThreshold:
      case SolErrorKind.kLengthOfRequestedSeedIsTooLong:
      case SolErrorKind.kLoaderCallChainIsTooDeep:
      case SolErrorKind.kNonNativeAccountCanOnlyBeClosedIfItsBalanceIsZero:
      case SolErrorKind.kOperationOverflowed:
      case SolErrorKind.kOwnerDoesNotMatch:
      case SolErrorKind.kProvidedAddressDoesNotMatchAddressedDerivedFromSeed:
      case SolErrorKind.kSimulationFailed:
      case SolErrorKind.kSimulationTimedOut:
      case SolErrorKind.kSpecifiedNonceDoesNotMatchStoredNonce:
      case SolErrorKind.kStateIsInvalidForRequestedOperation:
      case SolErrorKind.kStateIsUninitialized:
      case SolErrorKind.kStoredNonceIsStillInRecentBlockhashes:
      case SolErrorKind.kTheProvidedDecimalsValueDifferentFromTheMintDecimals:
      case SolErrorKind.kThisAccountMayNotBeUsedToPayTransactionFees:
      case SolErrorKind.kThisProgramMayNotBeUsedForExecutingInstructions:
      case SolErrorKind.kThisTokenMintCannotFreezeAccounts:
      case SolErrorKind.kThisTransactionHasAlreadyBeenProcessed:
      case SolErrorKind.kTransactionAddressTableLookupUsesAnInvalidIndex:
      // eslint-disable-next-line max-len
      case SolErrorKind.kTransactionContainsADuplicateInstructionThatIsNotAllowed:
      case SolErrorKind.kTransactionContainsAnInvalidAccountReference:
      case SolErrorKind.kTransactionDidNotPassSignatureVerification:
      case SolErrorKind.kTransactionFailedToSanitizeAccountsOffsetsCorrectly:
      // eslint-disable-next-line max-len
      case SolErrorKind.kTransactionLeavesAnAccountWithALowerBalanceThanRentExemptMinimum:
      case SolErrorKind.kTransactionLoadsAWritableAccountThatCannotBeWritten:
      case SolErrorKind.kTransactionLoadsAnAddressTableAccountThatDoesntExist:
      // eslint-disable-next-line max-len
      case SolErrorKind.kTransactionLoadsAnAddressTableAccountWithAnInvalidOwner:
      case SolErrorKind.kTransactionLoadsAnAddressTableAccountWithInvalidData:
      case SolErrorKind.kTransactionLockedTooManyAccounts:
      // eslint-disable-next-line max-len
      case SolErrorKind.kTransactionProcessingLeftAnAccountWithAnOutstandingBorrowedReference:
      case SolErrorKind.kTransactionRequiresAFeeButHasNoSignaturePresent:
      case SolErrorKind.kTransactionVersionIsUnsupported:
      case SolErrorKind.kTransactionWouldExceedAccountDataLimitWithinTheBlock:
      case SolErrorKind.kTransactionWouldExceedMaxAccountLimitWithinTheBlock:
      case SolErrorKind.kTransactionWouldExceedMaxBlockCostLimit:
      case SolErrorKind.kTransactionWouldExceedMaxVoteCostLimit:
      case SolErrorKind.kTransactionWouldExceedTotalAccountDataLimit:
      // eslint-disable-next-line max-len
      case SolErrorKind.kTransactionsAreCurrentlyDisabledDueToClusterMaintenance:
      case SolErrorKind.kUnknownError:
      default:
        return (
          error.humanReadableError ||
          getLocale('braveWalletSimulationUnexpectedError')
        )
    }
  }

  // EVM
  switch (error.kind) {
    case BlowfishEVMErrorKind.kTransactionReverted:
      return getLocale('braveWalletSimulationErrorTransactionReverted')

    // Known Unknowns
    case BlowfishEVMErrorKind.kTransactionError:
    case BlowfishEVMErrorKind.kSimulationFailed:
    case BlowfishEVMErrorKind.kUnknownError:
      return getLocale('braveWalletSimulationUnexpectedError')

    // Unknown error type
    default:
      return (
        error.humanReadableError ||
        getLocale('braveWalletSimulationUnexpectedError')
      )
  }
}
