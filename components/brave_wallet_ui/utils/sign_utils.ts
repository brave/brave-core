// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'

export const PERMIT_LIKE: readonly string[] = [
  // ERC-2612 and DAI-style
  'Permit',

  // Uniswap Permit2 (AllowanceTransfer)
  'PermitSingle',
  'PermitBatch',

  // Uniswap Permit2 (SignatureTransfer)
  'PermitTransferFrom',
  'PermitBatchTransferFrom',
  'PermitWitnessTransferFrom',
  'PermitBatchWitnessTransferFrom',

  // EIP-3009 (USDC and for x402 support)
  'TransferWithAuthorization',
  'ReceiveWithAuthorization',
  'CancelAuthorization',
]

export const PERMIT2_ADDRESS = '0x000000000022D473030F116DDeE9F6b43ac78BA3'

type TypedDataField = { name: string; type: string }

type EthSignTypedDataDomain = {
  verifyingContract?: string
}

function parseTypesJson(
  typesJson: string | undefined,
): Record<string, TypedDataField[]> | undefined {
  if (!typesJson) {
    return undefined
  }
  try {
    return JSON.parse(typesJson) as Record<string, TypedDataField[]>
  } catch {
    return undefined
  }
}

function parseDomain(
  domainJson: string | undefined,
): EthSignTypedDataDomain | undefined {
  if (!domainJson) {
    return undefined
  }
  try {
    return JSON.parse(domainJson) as EthSignTypedDataDomain
  } catch {
    return undefined
  }
}

export function isPermit2Domain(td: BraveWallet.EthSignTypedData): boolean {
  const domain = parseDomain(td.domainJson)
  return (
    domain?.verifyingContract?.toLowerCase() === PERMIT2_ADDRESS.toLowerCase()
  )
}

function fieldsMatchPermitShape(fields: TypedDataField[]): boolean {
  const names = new Set(fields.map((f) => f.name))

  const isPermitStyle =
    names.has('spender')
    && (names.has('value')
      || names.has('amount')
      || fields.some(
        (f) =>
          f.type === 'PermitDetails'
          || f.type === 'PermitDetails[]'
          || f.type === 'TokenPermissions'
          || f.type === 'TokenPermissions[]',
      ))

  // DAI-style: holder + spender + allowed(bool)
  const isDaiStyle =
    names.has('spender')
    && names.has('allowed')
    && fields.some((f) => f.name === 'allowed' && f.type === 'bool')

  // EIP-3009: from + to + value + validAfter + validBefore + nonce
  const isAuthorizationStyle =
    names.has('from')
    && names.has('to')
    && names.has('value')
    && names.has('validAfter')
    && names.has('validBefore')
    && names.has('nonce')

  return isPermitStyle || isDaiStyle || isAuthorizationStyle
}

function hasPermitShapeInTypes(
  types: Record<string, TypedDataField[]>,
): boolean {
  return Object.entries(types).some(([typeName, fields]) => {
    if (typeName === 'EIP712Domain' || !Array.isArray(fields)) {
      return false
    }
    return fieldsMatchPermitShape(fields)
  })
}

export function hasPermitShape(td: BraveWallet.EthSignTypedData): boolean {
  const types = parseTypesJson(td.typesJson)
  if (!types) {
    return false
  }
  return hasPermitShapeInTypes(types)
}

export function isPermitLikeEthSignTypedData(
  td: BraveWallet.EthSignTypedData | undefined,
): boolean {
  if (!td) {
    return false
  }
  return (
    (!!td.primaryType && PERMIT_LIKE.includes(td.primaryType))
    || hasPermitShape(td)
    || isPermit2Domain(td)
  )
}
