// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../constants/types'

export const BLOWFISH_URL_WARNING_KINDS = [
  BraveWallet.BlowfishWarningKind.kBlocklistedDomainCrossOrigin,
  BraveWallet.BlowfishWarningKind.kCopyCatDomain,
  BraveWallet.BlowfishWarningKind.kNonAsciiUrl,
  BraveWallet.BlowfishWarningKind.kSemiTrustedBlocklistDomain
] as const

export const BLOWFISH_WARNING_KINDS = [
  ...BLOWFISH_URL_WARNING_KINDS,
  BraveWallet.BlowfishWarningKind.kSuspectedMalicious,
  BraveWallet.BlowfishWarningKind.kKnownMalicious,
  BraveWallet.BlowfishWarningKind.kTransferringErc20ToOwnContract,
  BraveWallet.BlowfishWarningKind.kUnlimitedAllowanceToNfts,
  BraveWallet.BlowfishWarningKind.kBulkApprovalsRequest,
  BraveWallet.BlowfishWarningKind.kSetOwnerAuthority,
  BraveWallet.BlowfishWarningKind.kTrustedBlocklistDomain,
  BraveWallet.BlowfishWarningKind.kDanglingApproval,
  BraveWallet.BlowfishWarningKind.kTradeForNothing,
  BraveWallet.BlowfishWarningKind.kPermitUnlimitedAllowance,
  BraveWallet.BlowfishWarningKind.kPermitNoExpiration,
  BraveWallet.BlowfishWarningKind.kEthSignTxHash,
  BraveWallet.BlowfishWarningKind.kObfuscatedCode,
  BraveWallet.BlowfishWarningKind.kDevtoolsDisabled,
  BraveWallet.BlowfishWarningKind.kWhitelistedDomainCrossOrigin,
  BraveWallet.BlowfishWarningKind.kTooManyTransactions,
  BraveWallet.BlowfishWarningKind.kCompromisedAuthorityUpgrade,
  BraveWallet.BlowfishWarningKind.kPoisonedAddress,
  BraveWallet.BlowfishWarningKind.kApprovalToEOA
] as const
