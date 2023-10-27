// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../constants/types'

export const BLOWFISH_URL_WARNING_KINDS = [
  BraveWallet.BlowfishWarningKind.kBlocklistedDomainCrossOrigin,
  BraveWallet.BlowfishWarningKind.kCopyCatDomain,
  BraveWallet.BlowfishWarningKind.kCopyCatImageUnresponsiveDomain,
  BraveWallet.BlowfishWarningKind.kMultiCopyCatDomain,
  BraveWallet.BlowfishWarningKind.kNewDomain,
  BraveWallet.BlowfishWarningKind.kSemiTrustedBlocklistDomain
] as const

export const BLOWFISH_WARNING_KINDS = [
  ...BLOWFISH_URL_WARNING_KINDS,
  BraveWallet.BlowfishWarningKind.kApprovalToEOA,
  BraveWallet.BlowfishWarningKind.kBulkApprovalsRequest,
  BraveWallet.BlowfishWarningKind.kCompromisedAuthorityUpgrade,
  BraveWallet.BlowfishWarningKind.kDanglingApproval,
  BraveWallet.BlowfishWarningKind.kEthSignTxHash,
  BraveWallet.BlowfishWarningKind.kKnownMalicious,
  BraveWallet.BlowfishWarningKind.kPermitNoExpiration,
  BraveWallet.BlowfishWarningKind.kPermitUnlimitedAllowance,
  BraveWallet.BlowfishWarningKind.kPoisonedAddress,
  BraveWallet.BlowfishWarningKind.kReferencedOfacAddress,
  BraveWallet.BlowfishWarningKind.kSetOwnerAuthority,
  BraveWallet.BlowfishWarningKind.kSuspectedMalicious,
  BraveWallet.BlowfishWarningKind.kTooManyTransactions,
  BraveWallet.BlowfishWarningKind.kTradeForNothing,
  BraveWallet.BlowfishWarningKind.kTransferringErc20ToOwnContract,
  BraveWallet.BlowfishWarningKind.kTrustedBlocklistDomain,
  BraveWallet.BlowfishWarningKind.kUnknown,
  BraveWallet.BlowfishWarningKind.kUnlimitedAllowanceToNfts,
  BraveWallet.BlowfishWarningKind.kUnusualGasConsumption,
  BraveWallet.BlowfishWarningKind.kUserAccountOwnerChange,
  BraveWallet.BlowfishWarningKind.kWhitelistedDomainCrossOrigin
] as const
