// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../constants/types'
import { getGetMojoEnumValues } from '../../utils/enum-utils'

export const BLOWFISH_URL_WARNING_KINDS = [
  BraveWallet.BlowfishWarningKind.kBlocklistedDomainCrossOrigin,
  BraveWallet.BlowfishWarningKind.kCopyCatDomain,
  BraveWallet.BlowfishWarningKind.kCopyCatImageUnresponsiveDomain,
  BraveWallet.BlowfishWarningKind.kMultiCopyCatDomain,
  BraveWallet.BlowfishWarningKind.kNewDomain,
  BraveWallet.BlowfishWarningKind.kSemiTrustedBlocklistDomain,
  BraveWallet.BlowfishWarningKind.kTrustedBlocklistDomain
] as const

export const BLOWFISH_WARNING_KINDS = getGetMojoEnumValues(
  BraveWallet.BlowfishWarningKind
)
