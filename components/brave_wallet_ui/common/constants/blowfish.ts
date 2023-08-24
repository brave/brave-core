// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export const BLOWFISH_URL_WARNING_KINDS = [
  'BLOCKLISTED_DOMAIN_CROSS_ORIGIN',
  'COPY_CAT_DOMAIN',
  'NON_ASCII_URL',
  'SEMI_TRUSTED_BLOCKLIST_DOMAIN',
] as const

export const BLOWFISH_WARNING_KINDS = [
  ...BLOWFISH_URL_WARNING_KINDS,
  'SUSPECTED_MALICIOUS',
  'KNOWN_MALICIOUS',
  'TRANSFERRING_ERC20_TO_OWN_CONTRACT',
  'UNLIMITED_ALLOWANCE_TO_NFTS',
  'BULK_APPROVALS_REQUEST',
  'SET_OWNER_AUTHORITY',
  'TRUSTED_BLOCKLIST_DOMAIN',
  'DANGLING_APPROVAL',
  'TRADE_FOR_NOTHING',
  'PERMIT_UNLIMITED_ALLOWANCE',
  'PERMIT_NO_EXPIRATION',
  'ETH_SIGN_TX_HASH',
  'OBFUSCATED_CODE',
  'DEVTOOLS_DISABLED',
  'WHITELISTED_DOMAIN_CROSS_ORIGIN',
  'TOO_MANY_TRANSACTIONS',
  'COMPROMISED_AUTHORITY_UPGRADE',
  'POISONED_ADDRESS',
  'APPROVAL_TO_E_O_A',
] as const

/**
 * Domain/Site Warnings
*/
export type BlowfishURLWarningKind = typeof BLOWFISH_URL_WARNING_KINDS[number]

/**
 * Can be used to override specific warnings with your own custom versions.
 * Our UI should defer to the supplied message if the kind isn't recognized
 */
export type BlowfishWarningKind = typeof BLOWFISH_WARNING_KINDS[number]